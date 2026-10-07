//! Bounded network flight recording. Packet contents and keys never enter here.
pub use serde_json::{json, Value};
use std::collections::{BTreeMap, HashMap, VecDeque};
use std::fs::{self, File, OpenOptions};
use std::io::{BufWriter, Read, Seek, SeekFrom, Write};
use std::path::{Path, PathBuf};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::mpsc::{self, Receiver, SyncSender, TrySendError};
use std::sync::{Arc, Mutex, OnceLock};
use std::time::{Duration, Instant};

const HISTORY_MS: u64 = 120_000;
const HISTORY_BYTES: usize = 2 * 1024 * 1024;
const MEMORY_BYTES: usize = 16 * 1024 * 1024;
const AFTER_MS: u64 = 30_000;
const QUEUE: usize = 4096;
const FILE_BYTES: u64 = 8 * 1024 * 1024;
const REPORT_LIMIT: usize = 32;

/// PCs and module-relative offsets only; no stack memory or packet contents.
pub fn native_frames(frames: &[u64]) -> Value {
    #[cfg(windows)]
    unsafe {
        use std::ffi::c_void;
        #[link(name = "kernel32")]
        extern "system" {
            fn GetModuleHandleExW(flags: u32, address: *const u16, module: *mut *mut c_void)
                -> i32;
            fn GetModuleFileNameW(module: *mut c_void, path: *mut u16, size: u32) -> u32;
        }
        let mut result = Vec::new();
        for &pc in frames.iter().take(48).filter(|&&pc| pc != 0) {
            let mut module = std::ptr::null_mut();
            let mut path = [0u16; 1024];
            if GetModuleHandleExW(6, pc as *const u16, &mut module) != 0 {
                let n = GetModuleFileNameW(module, path.as_mut_ptr(), path.len() as u32) as usize;
                let path = String::from_utf16_lossy(&path[..n.min(path.len())]);
                result.push(json!({"pc":pc,"module":path.rsplit(['\\','/']).next(),"module_base":module as u64,"rva":pc.saturating_sub(module as u64)}));
            } else {
                result.push(json!({"pc":pc}));
            }
        }
        return json!(result);
    }
    #[cfg(not(windows))]
    json!(frames
        .iter()
        .take(48)
        .map(|pc| json!({"pc":pc}))
        .collect::<Vec<_>>())
}

pub fn native_stack() -> Value {
    #[cfg(windows)]
    unsafe {
        use std::ffi::c_void;
        #[link(name = "kernel32")]
        extern "system" {
            fn RtlCaptureStackBackTrace(
                skip: u32,
                count: u32,
                frames: *mut *mut c_void,
                hash: *mut u32,
            ) -> u16;
        }
        let mut frames = [std::ptr::null_mut::<c_void>(); 48];
        let count = RtlCaptureStackBackTrace(1, 48, frames.as_mut_ptr(), std::ptr::null_mut());
        return native_frames(
            &frames[..count as usize]
                .iter()
                .map(|p| *p as u64)
                .collect::<Vec<_>>(),
        );
    }
    #[cfg(not(windows))]
    json!([])
}

#[derive(Default, Clone)]
struct History {
    rows: VecDeque<Value>,
    bytes: usize,
    lengths: VecDeque<usize>,
    evicted: u64,
    limit: usize,
}

impl History {
    fn push(&mut self, row: Value) {
        let now = row["elapsed_ms"].as_u64().unwrap_or(0);
        let length = row.to_string().len();
        self.bytes += length;
        self.lengths.push_back(length);
        self.rows.push_back(row);
        let limit = self.limit.max(HISTORY_BYTES);
        while self.bytes > limit
            || self.rows.front().is_some_and(|r| {
                now.saturating_sub(r["elapsed_ms"].as_u64().unwrap_or(now)) > HISTORY_MS
            })
        {
            self.rows.pop_front().unwrap();
            self.bytes -= self.lengths.pop_front().unwrap();
            self.evicted += 1;
        }
    }
}

const STAGES: [&str; 14] = [
    "guest_submitted",
    "tunnel_queued",
    "tunnel_received",
    "udp_socket_sent",
    "udp_received",
    "guest_queued",
    "guest_consumed",
    "udp_rejected",
    "tunnel_written",
    "stream_written",
    "stream_received",
    "udp_wire_received",
    "udp_probe_sent",
    "udp_send_error",
];
const REJECTIONS: [&str; 6] = [
    "replay_or_outside_window",
    "authentication_failed",
    "truncated",
    "too_large",
    "short_ports",
    "other",
];

#[derive(Default)]
struct Accounting {
    events: [AtomicU64; STAGES.len()],
    bytes: [AtomicU64; STAGES.len()],
    sampled: [AtomicU64; STAGES.len()],
    windows: [AtomicU64; STAGES.len()],
    detail_until: AtomicU64,
    priority_dropped: AtomicU64,
    rejections: [AtomicU64; 6],
    flows: Mutex<HashMap<(usize, String, u16, u16), Traffic>>,
    flow_missed_events: AtomicU64,
    flow_missed_bytes: AtomicU64,
    transport: Mutex<Option<libp2p::metrics::Registry>>,
    protocol_start_totals: Value,
}

#[derive(Default)]
struct Traffic { events: u64, bytes: u64, payload_bytes: u64, max_bytes: u64, large_packets: u64 }

fn rate(bytes: u64, interval_ms: u64) -> Value {
    let per_second = bytes as f64 * 1000.0 / interval_ms.max(1) as f64;
    json!({"bytes":bytes,"bytes_per_second":per_second,"MB_per_second":per_second / 1_000_000.0,"Mbps":per_second * 8.0 / 1_000_000.0})
}

// Fixed labels: remote peers cannot grow the map with arbitrary protocol names
// or opcodes. Count at the common Protobuf codec, before recorder sampling.
const SIZE_BOUNDS: [u64; 8] = [128, 512, 1024, 4096, 8192, 16384, 65536, u64::MAX];
const SIZE_LABELS: [&str; 8] = ["0-128", "129-512", "513-1024", "1025-4096",
    "4097-8192", "8193-16384", "16385-65536", "above-65536"];
type ProtocolKey = (&'static str, &'static str, &'static str, &'static str);

#[derive(Default)]
struct MessageSizes {
    events: u64, bytes: u64, framed_bytes: u64, min: u64, max: u64,
    declared_max: u64, bins: [u64; 8],
}

#[derive(Default)]
struct ProtocolMeasurements { groups: BTreeMap<ProtocolKey, MessageSizes> }
// ponytail: one mutex for fixed process-wide groups; shard counters if measured contention matters.
static PROTOCOL_MEASUREMENTS: OnceLock<Mutex<ProtocolMeasurements>> = OnceLock::new();

impl ProtocolMeasurements {
    fn record(&mut self, row: quick_protobuf_codec::measurement::Observation) {
        use quick_protobuf_codec::measurement::{Direction, Outcome};
        let (protocol, rpc) = if row.message_type.starts_with("libp2p_kad::") {
            ("dht", match row.dht_rpc {
                Some(0) => "put_value", Some(1) => "get_value", Some(2) => "add_provider",
                Some(3) => "get_providers", Some(4) => "find_node", Some(5) => "ping", _ => "unknown",
            })
        } else if row.message_type.starts_with("libp2p_identify::") { ("identify", "identify") }
        else if row.message_type.starts_with("libp2p_relay::") {
            ("relay", if row.message_type.ends_with("HopMessage") { "hop" }
                else if row.message_type.ends_with("StopMessage") { "stop" } else { "other" })
        }
        else if row.message_type.starts_with("libp2p_autonat::") { ("autonat", "autonat") }
        else if row.message_type.starts_with("libp2p_dcutr::") { ("hole_punch", "hole_punch") }
        else { ("other_protobuf", "other") };
        let direction = match row.direction { Direction::Encoded => "encoded", Direction::Received => "received" };
        let outcome = match row.outcome {
            Outcome::Complete => "complete", Outcome::Oversized => "oversized",
            Outcome::Malformed => "malformed", Outcome::Incomplete => "incomplete",
        };
        let group = self.groups.entry((protocol, rpc, direction, outcome)).or_default();
        let size = row.payload_bytes as u64;
        group.min = if group.events == 0 { size } else { group.min.min(size) };
        group.events += 1;
        group.bytes += size;
        group.framed_bytes += size + row.prefix_bytes as u64;
        group.max = group.max.max(size);
        group.declared_max = group.declared_max.max(row.declared_payload_bytes.unwrap_or(0) as u64);
        group.bins[SIZE_BOUNDS.iter().position(|&bound| size <= bound).unwrap()] += 1;
    }

    fn snapshot(&self) -> Value {
        json!(self.groups.iter().map(|(&(protocol, rpc, direction, outcome), s)| {
            let bins: BTreeMap<_, _> = SIZE_LABELS.into_iter().zip(s.bins).collect();
            (format!("{protocol}/{rpc}/{direction}/{outcome}"), json!({
                "protocol":protocol,"rpc":rpc,"direction":direction,"outcome":outcome,
                "events":s.events,"bytes":s.bytes,"framed_bytes":s.framed_bytes,
                "min_payload_bytes":s.min,"max_payload_bytes":s.max,
                "max_declared_payload_bytes":s.declared_max,"size_bins":bins}))
        }).collect::<BTreeMap<_, _>>())
    }
}

pub(crate) fn install_protocol_measurement() {
    PROTOCOL_MEASUREMENTS.get_or_init(Mutex::default);
    let _ = quick_protobuf_codec::measurement::install_observer(|row| {
        if let Some(totals) = PROTOCOL_MEASUREMENTS.get() {
            totals.lock().unwrap_or_else(|e| e.into_inner()).record(row);
        }
    });
}

fn protocol_message_snapshot() -> Value {
    PROTOCOL_MEASUREMENTS.get().map(|totals|
        totals.lock().unwrap_or_else(|e| e.into_inner()).snapshot()).unwrap_or_else(|| json!({}))
}

fn protocol_message_rates(current: &Value, previous: &Value, interval_ms: u64) -> Vec<Value> {
    let mut rows = Vec::new();
    for (key, totals) in current.as_object().into_iter().flatten() {
        let old = &previous[key];
        let delta = |field: &str| totals[field].as_u64().unwrap_or(0)
            .saturating_sub(old[field].as_u64().unwrap_or(0));
        let events = delta("events");
        if events == 0 { continue; }
        let bytes = delta("bytes");
        let mut value = rate(bytes, interval_ms);
        for field in ["protocol", "rpc", "direction", "outcome"] { value[field] = totals[field].clone(); }
        value["events"] = json!(events);
        value["messages_per_second"] = json!(events as f64 * 1000.0 / interval_ms.max(1) as f64);
        value["framed_bytes"] = json!(delta("framed_bytes"));
        value["average_payload_bytes"] = json!(bytes as f64 / events as f64);
        value["total_events"] = totals["events"].clone();
        value["total_payload_bytes"] = totals["bytes"].clone();
        value["lifetime_min_payload_bytes"] = totals["min_payload_bytes"].clone();
        value["lifetime_max_payload_bytes"] = totals["max_payload_bytes"].clone();
        value["lifetime_max_declared_payload_bytes"] = totals["max_declared_payload_bytes"].clone();
        value["size_bins"] = json!(SIZE_LABELS.into_iter().map(|label|
            (label, totals["size_bins"][label].as_u64().unwrap_or(0)
                .saturating_sub(old["size_bins"][label].as_u64().unwrap_or(0))))
            .collect::<BTreeMap<_, _>>());
        rows.push(value);
    }
    rows.sort_by_key(|r| std::cmp::Reverse(r["bytes"].as_u64().unwrap_or(0)));
    rows
}
fn adapter_totals() -> Value {
    #[cfg(windows)]
    unsafe {
        use windows_sys::Win32::NetworkManagement::IpHelper::{GetIfTable2,FreeMibTable};
        let mut table = std::ptr::null_mut();
        let error = GetIfTable2(&mut table);
        if error != 0 || table.is_null() { return json!({"error":error}); }
        let mut rows = serde_json::Map::new();
        // The OS owns the table. Cap exported interfaces; never retain its memory.
        for row in std::slice::from_raw_parts((*table).Table.as_ptr(),(*table).NumEntries.min(128) as usize) {
            if row.OperStatus != 1 || row.Type == 24 { continue; }
            let end = row.Alias.iter().position(|c|*c==0).unwrap_or(row.Alias.len());
            rows.insert(row.InterfaceIndex.to_string(),json!({"name":String::from_utf16_lossy(&row.Alias[..end]),
                "type":row.Type,"tx_bytes":row.OutOctets,"rx_bytes":row.InOctets,
                "tx_errors":row.OutErrors,"rx_errors":row.InErrors,"tx_discards":row.OutDiscards,"rx_discards":row.InDiscards,
                "tx_link_bits_per_second":row.TransmitLinkSpeed,"rx_link_bits_per_second":row.ReceiveLinkSpeed}));
        }
        FreeMibTable(table.cast());
        return json!(rows);
    }
    #[cfg(not(windows))]
    json!({"unavailable":"Windows adapter counters"})
}

impl Accounting {
    // All stage totals are exact before sampling. Flow attribution never waits
    // on a producer; contention/capacity loss is explicitly counted below.
    fn traffic(&self, stage: usize, fields: &Value, bytes: u64) {
        if let Ok(mut flows) = self.flows.try_lock() {
            let key = (stage, fields["peer"].as_str().map(str::to_owned)
                .unwrap_or_else(|| fields["virtual_ip"].as_u64().map(|n|n.to_string())
                    .or_else(||fields["endpoint"].as_str().map(str::to_owned)).unwrap_or_default()),
                fields["src_port"].as_u64().unwrap_or(0) as u16,
                fields["dst_port"].as_u64().unwrap_or(0) as u16);
            if flows.len() < 256 || flows.contains_key(&key) {
                let flow = flows.entry(key).or_default();
                flow.events += 1; flow.bytes += bytes;
                flow.payload_bytes += fields["payload_bytes"].as_u64().unwrap_or(bytes);
                flow.max_bytes = flow.max_bytes.max(bytes);
                flow.large_packets += u64::from(bytes >= 1024);
                return;
            }
        }
        self.flow_missed_events.fetch_add(1, Ordering::Relaxed);
        self.flow_missed_bytes.fetch_add(bytes, Ordering::Relaxed);
    }

    fn bandwidth(&self, previous: &mut Value, interval_ms: u64) -> Value {
        let mut current = self.snapshot();
        let protocol_messages = protocol_message_rates(&current["protobuf_message_totals"],
            &previous["protobuf_message_totals"], interval_ms);
        let mut stages = serde_json::Map::new();
        for stage in STAGES {
            let bytes = current["stages"][stage]["bytes"].as_u64().unwrap_or(0)
                .saturating_sub(previous["stages"][stage]["bytes"].as_u64().unwrap_or(0));
            let mut value = rate(bytes, interval_ms);
            value["events"] = json!(current["stages"][stage]["events"].as_u64().unwrap_or(0)
                .saturating_sub(previous["stages"][stage]["events"].as_u64().unwrap_or(0)));
            stages.insert(stage.into(), value);
        }
        let mut flows: Vec<_> = self.flows.lock().unwrap_or_else(|e|e.into_inner()).drain()
            .map(|((stage, peer, src, dst), flow)| {
                let mut value = rate(flow.bytes, interval_ms);
                value["stage"] = json!(STAGES[stage]); value["peer_or_address"] = json!(peer);
                value["src_port"] = json!(src); value["dst_port"] = json!(dst);
                value["events"] = json!(flow.events); value["payload_bytes"] = json!(flow.payload_bytes);
                value["max_bytes"] = json!(flow.max_bytes); value["packets_at_least_1024_bytes"] = json!(flow.large_packets);
                value
            }).collect();
        flows.sort_by_key(|v| std::cmp::Reverse(v["bytes"].as_u64().unwrap_or(0)));
        let mut transport = Vec::new();
        let mut protocol_bytes = Vec::new();
        let mut totals = serde_json::Map::new();
        if let Some(registry) = self.transport.lock().unwrap_or_else(|e|e.into_inner()).as_ref() {
            let mut encoded = String::new();
            if prometheus_client::encoding::text::encode(&mut encoded, registry).is_ok() {
                for line in encoded.lines().filter(|l| !l.starts_with('#') && l.contains("bytes_total")) {
                    if let Some((key, bytes)) = line.rsplit_once(' ') {
                        if let Ok(bytes) = bytes.parse::<u64>() {
                            let mut value = rate(bytes.saturating_sub(previous["transport_stream_totals"][key].as_u64().unwrap_or(0)), interval_ms);
                            value["metric"] = json!(key); value["total_bytes"] = json!(bytes);
                            totals.insert(key.into(), json!(bytes));
                            // The registry preallocates many protocol combinations.
                            // Keep baselines, without logging hundreds of unused rows.
                            if bytes == 0 { continue; }
                            if key.starts_with("libp2p_bandwidth_bytes_total") { transport.push(value); }
                            else { protocol_bytes.push(value); }
                        }
                    }
                }
            }
        }
        current["transport_stream_totals"] = json!(totals);
        let adapters = adapter_totals();
        let mut adapter_rates = Vec::new();
        for (index, values) in adapters.as_object().into_iter().flatten().filter(|(_,v)|v.is_object()) {
            let mut value = values.clone();
            value["interface_index"] = json!(index);
            for direction in ["tx", "rx"] {
                let key = format!("{direction}_bytes");
                let count = values[&key].as_u64().unwrap_or(0);
                // New interfaces establish a baseline, not a lifetime-traffic spike.
                value[direction] = match previous["adapter_totals"][index][&key].as_u64() {
                    Some(old) if count>=old => rate(count-old,interval_ms),
                    _ => Value::Null,
                };
            }
            adapter_rates.push(value);
        }
        current["adapter_totals"] = adapters;
        *previous = current;
        json!({"interval_ms":interval_ms,"stages":stages,"flows":flows,
            "flow_detail_missed_events":self.flow_missed_events.swap(0, Ordering::Relaxed),
            "flow_detail_missed_bytes":self.flow_missed_bytes.swap(0, Ordering::Relaxed),
            "transport_streams":transport,
            "protobuf_messages":protocol_messages,
            "gossip_message_bytes":protocol_bytes,"adapters_all_processes":adapter_rates,
            "measurement":"separate_layers_do_not_sum; protobuf is process-wide complete/attempted frames, encoded is not socket-sent; libp2p streams exclude retransmits and transport headers; UDP socket bytes exclude IP/UDP headers"})
    }

    fn snapshot(&self) -> Value {
        let mut result = serde_json::Map::new();
        for (i, stage) in STAGES.iter().enumerate() {
            result.insert((*stage).into(), json!({"events":self.events[i].load(Ordering::Relaxed),
                "bytes":self.bytes[i].load(Ordering::Relaxed),"sampled_out":self.sampled[i].load(Ordering::Relaxed)}));
        }
        let reasons: serde_json::Map<String, Value> = REJECTIONS
            .iter()
            .enumerate()
            .map(|(i, r)| {
                (
                    r.to_string(),
                    json!(self.rejections[i].load(Ordering::Relaxed)),
                )
            })
            .collect();
        json!({"stages":result,"udp_rejection_reasons":reasons,
            "protobuf_message_totals":protocol_message_snapshot(),
            "priority_records_dropped":self.priority_dropped.load(Ordering::Relaxed)})
    }
    fn retain(&self, kind: &str, fields: &Value, elapsed: u64) -> bool {
        let Some(i) = STAGES.iter().position(|s| *s == kind) else {
            return true;
        };
        self.events[i].fetch_add(1, Ordering::Relaxed);
        if kind == "udp_rejected" {
            let reason = fields["reason"].as_str().unwrap_or("other");
            let at = REJECTIONS.iter().position(|r| *r == reason).unwrap_or(5);
            self.rejections[at].fetch_add(1, Ordering::Relaxed);
        }
        let bytes = fields["bytes"]
                .as_u64()
                .or_else(|| fields["wire_bytes"].as_u64())
                .unwrap_or(0);
        self.bytes[i].fetch_add(bytes, Ordering::Relaxed);
        self.traffic(i, fields, bytes);
        // Counts cover every event; detailed packet metadata has an explicit budget.
        // A retained queue record keeps its corresponding consume record.
        if kind == "guest_consumed" && fields["queued_record"].as_u64().is_some_and(|n| n != 0) {
            return true;
        }
        let second = elapsed / 1000 + 1;
        let limit = if elapsed < self.detail_until.load(Ordering::Relaxed) {
            64
        } else {
            16
        };
        let mut old = self.windows[i].load(Ordering::Relaxed);
        loop {
            let count = if old >> 32 == second {
                old & 0xffff_ffff
            } else {
                0
            };
            if count >= limit {
                self.sampled[i].fetch_add(1, Ordering::Relaxed);
                return false;
            }
            match self.windows[i].compare_exchange_weak(
                old,
                (second << 32) | (count + 1),
                Ordering::Relaxed,
                Ordering::Relaxed,
            ) {
                Ok(_) => return true,
                Err(current) => old = current,
            }
        }
    }
}

fn decision_kind(kind: &str) -> bool {
    kind != "runtime_memory_sample" && (kind.starts_with("game_")
        || kind.starts_with("runtime_")
        || kind.starts_with("punch_")
        || kind == "session_context")
}

fn memory_kind(row: &Value) -> bool { row["kind"] == "runtime_memory_sample" }

enum Message {
    Row(Value, bool),
    Context(Value),
    Flush(mpsc::Sender<()>),
}

/// No filesystem, network operation or waiting occurs on a producer thread.
pub struct Recorder {
    tx: Option<SyncSender<Message>>,
    urgent: Option<SyncSender<Message>>,
    dropped: Arc<AtomicU64>,
    polled: Arc<AtomicU64>,
    accounting: Arc<Accounting>,
    started: Instant,
    seq: AtomicU64,
}

impl Recorder {
    pub fn start(data_dir: &Path, title: u32, peer: &str) -> Arc<Self> {
        let started = Instant::now();
        let dropped = Arc::new(AtomicU64::new(0));
        let polled = Arc::new(AtomicU64::new(0));
        let accounting = Arc::new(Accounting { protocol_start_totals:protocol_message_snapshot(),
            ..Accounting::default() });
        let disabled =
            title == 0 || std::env::var("XRECOMP_NET_DIAGNOSTICS").is_ok_and(|v| v == "0");
        let (tx, urgent) = if disabled {
            (None, None)
        } else {
            install_protocol_measurement();
            let (tx, rx) = mpsc::sync_channel(QUEUE);
            let (urgent, urgent_rx) = mpsc::sync_channel(1024);
            let directory = std::env::var_os("XRECOMP_NET_DIAGNOSTICS_DIR")
                .map(PathBuf::from)
                .unwrap_or_else(|| {
                    std::env::current_exe()
                        .ok()
                        .and_then(|p| p.parent().map(|p| p.join("NetworkLogs")))
                        .unwrap_or_else(|| data_dir.join("diagnostics"))
                });
            let fallback = data_dir.join("diagnostics");
            let dropped = dropped.clone();
            let polled = polled.clone();
            let accounting_worker = accounting.clone();
            let peer = peer.to_owned();
            match std::thread::Builder::new()
                .name("rexnet-recorder".into())
                .spawn(move || {
                    let directory = if fs::create_dir_all(&directory).is_ok() {
                        directory
                    } else {
                        fallback
                    };
                    worker(
                        directory,
                        title,
                        peer,
                        rx,
                        urgent_rx,
                        dropped,
                        polled,
                        accounting_worker,
                        started,
                    );
                }) {
                Ok(_) => (Some(tx), Some(urgent)),
                Err(error) => {
                    eprintln!("[NETDIAG] recorder unavailable: {error}");
                    (None, None)
                }
            }
        };
        Arc::new(Self {
            tx,
            urgent,
            dropped,
            polled,
            accounting,
            started,
            seq: AtomicU64::new(0),
        })
    }

    pub fn enabled(&self) -> bool {
        self.tx.is_some()
    }

    pub(crate) fn transport_registry(&self, registry: libp2p::metrics::Registry) {
        *self.accounting.transport.lock().unwrap_or_else(|e|e.into_inner()) = Some(registry);
    }

    #[cfg(test)]
    pub(crate) fn test_totals(&self) -> Value { self.accounting.snapshot() }

    #[cfg(test)]
    pub(crate) fn test_bandwidth(&self) -> Value { self.accounting.bandwidth(&mut Value::Null,1000) }

    fn send(&self, message: Message) {
        // Reserve capacity for disconnect evidence even if packet telemetry
        // fills the normal queue. Both queues stay bounded and nonblocking.
        let priority = match &message {
            Message::Row(row, incident) => {
                *incident
                    || row["kind"]
                        .as_str()
                        .is_some_and(|k| decision_kind(k) || k.ends_with("_health") || k == "runtime_memory_sample")
            }
            Message::Context(_) => true,
            _ => false,
        };
        let queue = if priority { &self.urgent } else { &self.tx };
        if let Some(tx) = queue {
            if let Err(TrySendError::Full(_) | TrySendError::Disconnected(_)) = tx.try_send(message)
            {
                self.dropped.fetch_add(1, Ordering::Relaxed);
                if priority {
                    self.accounting
                        .priority_dropped
                        .fetch_add(1, Ordering::Relaxed);
                }
            }
        }
    }

    pub fn record(&self, kind: &str, fields: Value) -> u64 {
        self.row(kind, fields, false)
    }

    pub fn incident(&self, kind: &str, fields: Value) {
        self.row(kind, fields, true);
    }

    fn row(&self, kind: &str, fields: Value, trigger: bool) -> u64 {
        if self.tx.is_none() {
            return 0;
        }
        let elapsed = self.started.elapsed().as_millis() as u64;
        if trigger {
            self.accounting
                .detail_until
                .store(elapsed + AFTER_MS, Ordering::Relaxed);
        }
        if !trigger && !self.accounting.retain(kind, &fields, elapsed) {
            return 0;
        }
        let seq = self.seq.fetch_add(1, Ordering::Relaxed) + 1;
        self.send(Message::Row(
            json!({"seq":seq, "utc_ms":crate::notice::unix_ms(),
            "elapsed_ms":self.started.elapsed().as_millis() as u64, "kind":kind, "fields":fields}),
            trigger,
        ));
        seq
    }

    pub fn polled(&self) {
        self.polled.store(
            self.started.elapsed().as_millis() as u64 + 1,
            Ordering::Relaxed,
        );
    }

    pub fn context(&self, value: Value) {
        self.send(Message::Context(value));
    }

    /// Best effort on normal exit; a stalled disk can never hold shutdown indefinitely.
    pub fn flush(&self) {
        if let Some(tx) = &self.tx {
            let (done, finished) = mpsc::channel();
            if tx.try_send(Message::Flush(done)).is_ok() {
                let _ = finished.recv_timeout(Duration::from_millis(250));
            }
        }
    }
}

struct Incident {
    path: PathBuf,
    id: String,
    reason: String,
    deadline: u64,
    before: History,
    health_before: History,
    after: History,
    health_after: History,
    decision_before: History,
    decision_after: History,
    memory_before: History,
    memory_after: History,
    position: (u64, u64),
}

impl Incident {
    fn capture(&mut self, row: Value) {
        // The incident queue can overtake packet telemetry. Classify by the
        // producer's timestamp and sequence, rather than by arrival here.
        let before = position(&row) <= self.position;
        if memory_kind(&row) {
            if before { self.memory_before.push(row); } else { self.memory_after.push(row); }
            return;
        }
        if is_health(&row) {
            if before {
                self.health_before.push(row.clone());
            } else {
                self.health_after.push(row.clone());
            }
        }
        if row["kind"].as_str().is_some_and(decision_kind) {
            if before {
                self.decision_before.push(row.clone());
            } else {
                self.decision_after.push(row.clone());
            }
        }
        if before {
            self.before.push(row);
        } else {
            self.after.push(row);
        }
    }

    fn report(&self, header: &Value, dropped: u64, complete: bool) -> Value {
        json!({"schema":3,"incident_id":self.id,"session":header,"reason":self.reason,
            "complete":complete,"records_dropped":dropped,"history_evicted":self.before.evicted,
            "after_evicted":self.after.evicted,"before":ordered(&self.before),"after":ordered(&self.after),
            "health_before":ordered(&self.health_before),"health_after":ordered(&self.health_after),
            "decision_before":ordered(&self.decision_before),"decision_after":ordered(&self.decision_after),
            "decision_before_evicted":self.decision_before.evicted,"decision_after_evicted":self.decision_after.evicted,
            "memory_before":ordered(&self.memory_before),"memory_after":ordered(&self.memory_after),
            "memory_before_evicted":self.memory_before.evicted,"memory_after_evicted":self.memory_after.evicted,
            "main_log_snapshot":header["main_log_snapshot"],"traffic_totals":header["traffic_totals"],
            "health_before_evicted":self.health_before.evicted,
            "health_after_evicted":self.health_after.evicted})
    }

    fn save(&self, header: &Value, dropped: u64, complete: bool) -> std::io::Result<()> {
        let temporary = self.path.with_extension("tmp");
        let mut file = File::create(&temporary)?;
        serde_json::to_writer(&mut file, &self.report(header, dropped, complete))?;
        file.flush()?;
        // Replace only after the whole snapshot is written; a failed write
        // leaves the previous report available, including on Windows.
        fs::rename(temporary, &self.path)
    }
}

fn position(row: &Value) -> (u64, u64) {
    (
        row["elapsed_ms"].as_u64().unwrap_or(0),
        row["seq"].as_u64().unwrap_or(0),
    )
}

fn ordered(history: &History) -> Vec<&Value> {
    let mut rows: Vec<_> = history.rows.iter().collect();
    rows.sort_by_key(|row| position(row));
    rows
}

fn split_history(history: &History, boundary: (u64, u64)) -> (History, History) {
    let mut before = History { limit:history.limit, ..History::default() };
    let mut after = History { limit:history.limit, ..History::default() };
    for row in &history.rows {
        if position(row) <= boundary {
            before.push(row.clone());
        } else {
            after.push(row.clone());
        }
    }
    before.evicted += history.evicted;
    (before, after)
}

fn is_health(row: &Value) -> bool {
    row["kind"]
        .as_str()
        .is_some_and(|kind| kind.ends_with("_health"))
}

fn write_log(
    file: &mut Option<BufWriter<File>>,
    row: &Value,
    written: &mut u64,
    dropped: &AtomicU64,
) {
    let text = row.to_string();
    *written += text.len() as u64 + 1;
    let result = file.as_mut().map(|file| writeln!(file, "{text}"));
    if let Some(Err(ref error)) = result {
        eprintln!("[NETDIAG] session log write failed: {error}");
        *file = None;
    }
    if !matches!(result, Some(Ok(()))) {
        dropped.fetch_add(1, Ordering::Relaxed);
    }
}

fn open_log(directory: &Path, prefix: &str, chunk: u64) -> Option<BufWriter<File>> {
    match OpenOptions::new()
        .create_new(true)
        .write(true)
        .open(directory.join(format!("{prefix}-{chunk}.jsonl")))
    {
        Ok(file) => Some(BufWriter::with_capacity(64 * 1024, file)),
        Err(error) => {
            eprintln!("[NETDIAG] session log open failed: {error}");
            None
        }
    }
}

fn flush_log(file: &mut Option<BufWriter<File>>, dropped: &AtomicU64) {
    if let Some(Err(error)) = file.as_mut().map(|file| file.flush()) {
        eprintln!("[NETDIAG] session log flush failed: {error}");
        dropped.fetch_add(1, Ordering::Relaxed);
        *file = None;
    }
}

fn prune(directory: &Path, suffix: &str, keep: usize) {
    let Ok(entries) = fs::read_dir(directory) else {
        return;
    };
    let mut paths: Vec<_> = entries
        .filter_map(Result::ok)
        .filter(|e| {
            e.file_name().to_string_lossy().starts_with("netdiag-")
                && e.path().extension().is_some_and(|ext| ext == suffix)
                && e.file_type().is_ok_and(|t| t.is_file())
        })
        .map(|e| (e.metadata().and_then(|m| m.modified()).ok(), e.path()))
        .collect();
    paths.sort_by_key(|(time, _)| *time);
    let remove = paths.len().saturating_sub(keep);
    for (_, path) in paths.into_iter().take(remove) {
        let _ = fs::remove_file(path);
    }
}

// Copy only the registered process's log, on this I/O worker. Preserve header
// and the last 8 MiB; never replace a saved copy with a different launch's log.
fn snapshot_main_log(directory: &Path, prefix: &str, header: &mut Value) {
    let context = &header["context"];
    let Some(path) = context["runtime"]["main_log_path"].as_str() else {
        return;
    };
    let Some(pid) = context["runtime"]["process_id"].as_u64() else {
        return;
    };
    let result = (|| -> std::io::Result<Value> {
        let mut source = File::open(path)?;
        let size = source.metadata()?.len();
        let mut first = vec![0; size.min(65536) as usize];
        source.read_exact(&mut first)?;
        let text = String::from_utf8_lossy(&first);
        let identity = format!("pid={pid} =====");
        if !text.contains(&identity)
            || !context["build"]
                .as_str()
                .is_some_and(|build| text.contains(build))
        {
            return Err(std::io::Error::other(
                "main log belongs to a different process or build",
            ));
        }
        let name = format!("{prefix}-main.log");
        let temporary = directory.join(format!("{prefix}-main.log.tmp"));
        let mut output = File::create(&temporary)?;
        output.write_all(&first)?;
        let tail_start = size.saturating_sub(8 * 1024 * 1024).max(first.len() as u64);
        if tail_start > first.len() as u64 {
            output.write_all(
                b"\n[NETDIAG] middle of main log omitted; header and tail preserved\n",
            )?;
        }
        source.seek(SeekFrom::Start(tail_start))?;
        std::io::copy(&mut source.take(8 * 1024 * 1024), &mut output)?;
        output.flush()?;
        // A relaunch could truncate the source while its tail is being read.
        let mut verify = File::open(path)?;
        let mut check = vec![0; first.len()];
        verify.read_exact(&mut check)?;
        if !String::from_utf8_lossy(&check).contains(&identity) {
            return Err(std::io::Error::other("main log was replaced while copying"));
        }
        fs::rename(temporary, directory.join(&name))?;
        Ok(
            json!({"file":name,"source_bytes":size,"middle_omitted":tail_start>first.len() as u64,
            "process_id":pid,"updated_utc_ms":crate::notice::unix_ms()}),
        )
    })();
    match result {
        Ok(value) => {
            header["main_log_snapshot"] = value;
            prune(directory, "log", REPORT_LIMIT);
        }
        Err(error) => {
            header["main_log_snapshot"]["last_error"] = json!(error.to_string());
        }
    }
}

/// Startup recovery for a network session killed before its worker could save.
/// Called before the existing main log is truncated, never on the game loop.
pub fn preserve_previous_log(path: &Path, directory: &Path) -> std::io::Result<bool> {
    let mut input = match File::open(path) {
        Ok(input) => input,
        Err(e) if e.kind() == std::io::ErrorKind::NotFound => return Ok(false),
        Err(e) => return Err(e),
    };
    let mut head = vec![0; input.metadata()?.len().min(65536) as usize];
    input.read_exact(&mut head)?;
    let text = String::from_utf8_lossy(&head);
    if !text.contains("[NETDIAG]") {
        return Ok(false);
    }
    let Some(start) = text
        .lines()
        .find(|l| l.starts_with("===== xrecomp session start"))
    else {
        return Ok(false);
    };
    let Some(pid) = start
        .split("pid=")
        .nth(1)
        .and_then(|s| s.split_whitespace().next())
        .and_then(|s| s.parse::<u32>().ok())
    else {
        return Ok(false);
    };
    let Some(build) = text.lines().find(|l| l.starts_with("build:")) else {
        return Ok(false);
    };
    fs::create_dir_all(directory)?;
    let prefix = format!(
        "netdiag-{}-recovered-{:016x}",
        crate::notice::unix_ms(),
        rand::random::<u64>()
    );
    let mut header = json!({"context":{"build":build,"runtime":{"main_log_path":path,"process_id":pid}},"main_log_snapshot":{}});
    snapshot_main_log(directory, &prefix, &mut header);
    if header["main_log_snapshot"]["file"].is_string() {
        Ok(true)
    } else {
        Err(std::io::Error::other(
            header["main_log_snapshot"]["last_error"]
                .as_str()
                .unwrap_or("previous main log could not be saved")
                .to_owned(),
        ))
    }
}

fn worker(
    directory: PathBuf,
    title: u32,
    peer: String,
    rx: Receiver<Message>,
    urgent_rx: Receiver<Message>,
    dropped: Arc<AtomicU64>,
    polled: Arc<AtomicU64>,
    accounting: Arc<Accounting>,
    started: Instant,
) {
    if let Err(error) = fs::create_dir_all(&directory) {
        eprintln!("[NETDIAG] cannot create diagnostics directory: {error}");
        return;
    }
    use rand::RngCore;
    let mut random = [0; 16];
    if rand::rngs::OsRng.try_fill_bytes(&mut random).is_err() {
        eprintln!("[NETDIAG] randomness unavailable; recorder disabled");
        return;
    }
    let launch: String = random.iter().map(|b| format!("{b:02x}")).collect();
    let prefix = format!("netdiag-{}-{launch}", crate::notice::unix_ms());
    let mut header = json!({"launch_id":launch,"title_id":format!("{title:08x}"),
        "peer_id":peer,"started_utc_ms":crate::notice::unix_ms(),"context":{},"main_log_snapshot":{},"traffic_totals":{}});
    let mut history = History::default();
    // Preserve the two-minute health timeline independently of packet bursts.
    let mut health_history = History { limit:MEMORY_BYTES, ..History::default() };
    let mut decision_history = History::default();
    let mut memory_history = History { limit:MEMORY_BYTES, ..History::default() };
    let mut active: Option<Incident> = None;
    let mut incident_seq = 0;
    let mut last_poll_report = 0;
    // A recreated engine must not report traffic from an earlier engine in
    // this process as a new startup spike. Capture the baseline before spawn.
    let mut last_bandwidth = json!({"protobuf_message_totals":accounting.protocol_start_totals});
    let mut last_bandwidth_log = 0;
    let mut last_loop = 0;
    let mut worker_gap = 0;
    let mut last_checkpoint = 0;
    let mut chunk = 0;
    let mut written = 0u64;
    let mut file = open_log(&directory, &prefix, chunk);
    let mut deferred_flush = None;
    write_log(
        &mut file,
        &json!({"kind":"session_start","session":header}),
        &mut written,
        &dropped,
    );
    prune(&directory, "jsonl", 4);
    eprintln!("[NETDIAG] recording metadata in {}", directory.display());
    loop {
        let mut message = urgent_rx.try_recv().map_err(|_| ()).or_else(|_| {
            if let Some(done) = deferred_flush.take() {
                Ok(Message::Flush(done))
            } else {
                rx.recv_timeout(Duration::from_millis(100))
            }
        });
        // A normal-queue flush may wake recv before its earlier urgent row.
        // Drain that reserved queue before acknowledging the flush barrier.
        if matches!(&message, Ok(Message::Flush(_))) {
            if let Ok(priority) = urgent_rx.try_recv() {
                if let Ok(Message::Flush(done)) = message {
                    deferred_flush = Some(done);
                }
                message = Ok(priority);
            }
        }
        let now = started.elapsed().as_millis() as u64;
        worker_gap = worker_gap.max(now.saturating_sub(last_loop));
        last_loop = now;
        let closing = matches!(message, Err(mpsc::RecvTimeoutError::Disconnected));
        let mut flush = None;
        let mut row = None;
        let mut trigger = false;
        match message {
            Ok(Message::Row(value, incident)) => {
                row = Some(value);
                trigger = incident;
            }
            Ok(Message::Context(value)) => {
                if let Some(values) = value.as_object() {
                    for (key, value) in values {
                        header["context"][key] = value.clone();
                    }
                }
                let context_row =
                    json!({"kind":"session_context","elapsed_ms":now,"fields":header["context"]});
                decision_history.push(context_row.clone());
                if let Some(incident) = &mut active {
                    incident.capture(context_row);
                }
                write_log(
                    &mut file,
                    &json!({"kind":"session_context","session":header}),
                    &mut written,
                    &dropped,
                );
            }
            Ok(Message::Flush(done)) => flush = Some(done),
            Err(_) => {}
        }
        let health = if now.saturating_sub(last_poll_report) >= 1000 {
            let bandwidth = accounting.bandwidth(&mut last_bandwidth, now.saturating_sub(last_poll_report));
            let spike = bandwidth["stages"]["udp_socket_sent"]["bytes_per_second"].as_f64().unwrap_or(0.0)>=1_000_000.0
                || bandwidth["stages"]["udp_wire_received"]["bytes_per_second"].as_f64().unwrap_or(0.0)>=1_000_000.0
                || bandwidth["transport_streams"].as_array().is_some_and(|rows| rows.iter().any(|r|r["bytes_per_second"].as_f64().unwrap_or(0.0)>=1_000_000.0));
            if now.saturating_sub(last_bandwidth_log)>=30_000 || (spike && now.saturating_sub(last_bandwidth_log)>=1000)
                || (last_bandwidth_log==0 && bandwidth["protobuf_messages"].as_array().is_some_and(|r| !r.is_empty())) {
                last_bandwidth_log=now;
                eprintln!("[NET-BANDWIDTH] UDP tx={} MB/s rx={} MB/s; libp2p streams={}; largest stage/peer/port buckets={}",
                    bandwidth["stages"]["udp_socket_sent"]["MB_per_second"],bandwidth["stages"]["udp_wire_received"]["MB_per_second"],
                    json!(bandwidth["transport_streams"].as_array().unwrap().iter().filter(|r|r["bytes"].as_u64().unwrap_or(0)>0).collect::<Vec<_>>()),
                    json!(bandwidth["flows"].as_array().unwrap().iter().take(3).collect::<Vec<_>>()));
                eprintln!("[NET-PROTOCOL] largest Protobuf message groups={}",
                    json!(bandwidth["protobuf_messages"].as_array().unwrap().iter().take(3).collect::<Vec<_>>()));
            }
            last_poll_report = now;
            let last = polled.load(Ordering::Relaxed);
            Some(
                json!({"elapsed_ms":now,"utc_ms":crate::notice::unix_ms(),"kind":"recorder_health",
                "fields":{"bandwidth":bandwidth,"traffic_totals":accounting.snapshot(),"records_dropped":dropped.load(Ordering::Relaxed),
                "worker_loop_gap_ms":worker_gap,"wall_clock_shift_ms":crate::notice::unix_ms() as i64-now as i64-header["started_utc_ms"].as_i64().unwrap_or(0),
                "game_polled":last!=0,"game_poll_gap_ms":if last==0 {Value::Null} else {json!(now.saturating_sub(last-1))}}}),
            )
        } else {
            None
        };
        if let Some(health) = health {
            worker_gap = 0;
            write_log(&mut file, &health, &mut written, &dropped);
            if let Some(incident) = &mut active {
                incident.capture(health.clone());
            }
            health_history.push(health.clone());
            history.push(health);
            flush_log(&mut file, &dropped);
        }
        if let Some(row) = row {
            write_log(&mut file, &row, &mut written, &dropped);
            if memory_kind(&row) {
                if let Some(incident) = &mut active { incident.capture(row.clone()); }
                memory_history.push(row);
            } else {
                if row["kind"].as_str().is_some_and(decision_kind) {
                    decision_history.push(row.clone());
                }
                if is_health(&row) {
                    health_history.push(row.clone());
                }
                if let Some(incident) = &mut active {
                    incident.capture(row.clone());
                }
                history.push(row);
                if trigger && active.is_none() {
                    incident_seq += 1;
                    let id = format!("{launch}-{incident_seq}");
                    let reason = history.rows.back().unwrap()["kind"]
                        .as_str()
                        .unwrap_or("incident")
                        .to_owned();
                    let position = position(history.rows.back().unwrap());
                    let (before, after) = split_history(&history, position);
                    let (health_before, health_after) = split_history(&health_history, position);
                    let (decision_before, decision_after) = split_history(&decision_history, position);
                    let (memory_before, memory_after) = split_history(&memory_history, position);
                    snapshot_main_log(&directory, &prefix, &mut header);
                    header["traffic_totals"] = accounting.snapshot();
                    active = Some(Incident {
                        path: directory.join(format!("{prefix}-incident-{incident_seq}.json")),
                        id,
                        reason,
                        deadline: now + AFTER_MS,
                        before,
                        health_before,
                        after,
                        health_after,
                        decision_before,
                        decision_after,
                        memory_before,
                        memory_after,
                        position,
                    });
                    last_checkpoint = now;
                    prune(&directory, "json", REPORT_LIMIT - 1);
                    if let Some(incident) = &active {
                        if let Err(error) =
                            incident.save(&header, dropped.load(Ordering::Relaxed), false)
                        {
                            eprintln!("[NETDIAG] incident save failed: {error}");
                        }
                    }
                }
            }
        }
        if written >= FILE_BYTES {
            flush_log(&mut file, &dropped);
            chunk += 1;
            written = 0;
            file = open_log(&directory, &prefix, chunk);
            write_log(
                &mut file,
                &json!({"kind":"session_start","session":header}),
                &mut written,
                &dropped,
            );
            prune(&directory, "jsonl", 4);
        }
        let finished = active
            .as_ref()
            .is_some_and(|i| now >= i.deadline || closing || flush.is_some());
        if !finished && now.saturating_sub(last_checkpoint) >= 5000 {
            last_checkpoint = now;
            if let Some(incident) = &active {
                snapshot_main_log(&directory, &prefix, &mut header);
                header["traffic_totals"] = accounting.snapshot();
                if let Err(error) = incident.save(&header, dropped.load(Ordering::Relaxed), false) {
                    eprintln!("[NETDIAG] incident checkpoint failed: {error}");
                }
            }
        }
        if finished {
            snapshot_main_log(&directory, &prefix, &mut header);
            header["traffic_totals"] = accounting.snapshot();
            let incident = active.take().unwrap();
            let complete = now >= incident.deadline;
            match incident.save(&header, dropped.load(Ordering::Relaxed), complete) {
                Ok(()) => eprintln!(
                    "[NETDIAG] incident saved: {} complete={complete}",
                    incident.path.display()
                ),
                Err(error) => eprintln!("[NETDIAG] incident save failed: {error}"),
            }
        }
        if let Some(done) = flush {
            if incident_seq > 0 {
                snapshot_main_log(&directory, &prefix, &mut header);
            }
            flush_log(&mut file, &dropped);
            let _ = done.send(());
        }
        if closing {
            flush_log(&mut file, &dropped);
            break;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    #[cfg(windows)]
    fn native_incident_frames_have_module_offsets_for_the_exact_pdb() {
        let stack = native_stack();
        let frames = stack.as_array().unwrap();
        assert!(!frames.is_empty() && frames.len() <= 48);
        assert!(frames.iter().any(|frame| frame["module"]
            .as_str()
            .is_some_and(|m| m.ends_with(".exe"))));
        for frame in frames.iter().filter(|f| f["module_base"].is_number()) {
            assert_eq!(
                frame["pc"].as_u64().unwrap() - frame["module_base"].as_u64().unwrap(),
                frame["rva"].as_u64().unwrap()
            );
        }
    }

    #[test]
    fn startup_recovers_only_a_network_log_and_keeps_a_bounded_header_and_tail() {
        let directory = test_directory();
        let path = directory.join("Log_File_For_Tera.log");
        fs::write(
            &path,
            "===== xrecomp session start test pid=42 =====\nbuild: test\nordinary session\n",
        )
        .unwrap();
        assert!(!preserve_previous_log(&path, &directory).unwrap());
        let mut source = File::create(&path).unwrap();
        source.write_all(b"===== xrecomp session start test pid=42 =====\nbuild: test\n[NETDIAG] recording\nHEADER_FAULT\n").unwrap();
        source.write_all(&vec![b'x'; 10 * 1024 * 1024]).unwrap();
        source.write_all(b"\nLAST_FAULT\n").unwrap();
        drop(source);
        assert!(preserve_previous_log(&path, &directory).unwrap());
        let saved = fs::read_dir(&directory)
            .unwrap()
            .filter_map(Result::ok)
            .find(|e| {
                e.file_name().to_string_lossy().starts_with("netdiag-")
                    && e.path().extension().is_some_and(|e| e == "log")
            })
            .unwrap()
            .path();
        let bytes = fs::read(saved).unwrap();
        let body = String::from_utf8_lossy(&bytes);
        assert!(body.contains("HEADER_FAULT") && body.ends_with("LAST_FAULT\n"));
        assert!(bytes.len() < 8 * 1024 * 1024 + 65536 + 256);
        assert!(body.contains("middle of main log omitted"));
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn sampled_rejections_keep_every_reason_and_wire_byte_in_the_totals() {
        let accounting = Accounting::default();
        for _ in 0..1000 {
            accounting.retain(
                "udp_rejected",
                &json!({"reason":"authentication_failed","wire_bytes":64}),
                0,
            );
        }
        for _ in 0..10 {
            accounting.retain(
                "udp_rejected",
                &json!({"reason":"replay_or_outside_window","wire_bytes":32}),
                0,
            );
        }
        let value = accounting.snapshot();
        assert_eq!(value["stages"]["udp_rejected"]["events"], 1010);
        assert_eq!(value["stages"]["udp_rejected"]["bytes"], 64320);
        assert_eq!(
            value["udp_rejection_reasons"]["authentication_failed"],
            1000
        );
        assert_eq!(
            value["udp_rejection_reasons"]["replay_or_outside_window"],
            10
        );
        assert_eq!(value["stages"]["udp_rejected"]["sampled_out"], 994);
        assert!(accounting.retain("guest_consumed", &json!({"queued_record":99,"bytes":12}), 0));
    }

    #[test]
    fn transport_counters_are_rates_and_adapter_first_sample_is_a_baseline() {
        use prometheus_client::{metrics::counter::Counter,registry::Unit};
        let accounting = Accounting::default();
        let mut registry = libp2p::metrics::Registry::default();
        let counter: Counter = Counter::default();
        registry.sub_registry_with_prefix("libp2p").register_with_unit("bandwidth","test",Unit::Bytes,counter.clone());
        let gossip: Counter = Counter::default();
        registry.register("topic_msg_sent_bytes","test",gossip.clone());
        *accounting.transport.lock().unwrap() = Some(registry);
        counter.inc_by(1234); gossip.inc_by(32);
        let mut previous = Value::Null;
        let first = accounting.bandwidth(&mut previous,1000);
        assert_eq!(first["transport_streams"][0]["bytes"],1234);
        assert_eq!(first["gossip_message_bytes"][0]["bytes"],32);
        for row in first["adapters_all_processes"].as_array().unwrap() {
            assert_eq!(row["tx"],Value::Null); assert_eq!(row["rx"],Value::Null);
        }
        counter.inc_by(1000);
        let next = accounting.bandwidth(&mut previous,2000);
        assert_eq!(next["transport_streams"][0]["bytes_per_second"],500.0);
        assert_eq!(next["gossip_message_bytes"][0]["bytes"],0);
    }

    #[test]
    fn protocol_message_sizes_keep_rates_bins_and_rejections_separate() {
        use quick_protobuf_codec::measurement::{Direction, Observation, Outcome};
        let row = Observation { message_type:"libp2p_kad::proto::Message",
            direction:Direction::Received, outcome:Outcome::Complete,
            payload_bytes:100, prefix_bytes:1, declared_payload_bytes:Some(100), dht_rpc:Some(3) };
        let mut totals = ProtocolMeasurements::default();
        totals.record(row);
        let before = totals.snapshot();
        totals.record(Observation { payload_bytes:8193, prefix_bytes:2,
            declared_payload_bytes:Some(8193), ..row });
        totals.record(Observation { outcome:Outcome::Oversized, payload_bytes:0,
            prefix_bytes:3, declared_payload_bytes:Some(20_000), ..row });
        totals.record(Observation { message_type:"libp2p_identify::proto::Identify",
            direction:Direction::Encoded, dht_rpc:None, ..row });
        let after = totals.snapshot();
        let rates = protocol_message_rates(&after, &before, 2000);
        let good = rates.iter().find(|r| r["protocol"]=="dht" && r["outcome"]=="complete").unwrap();
        assert_eq!(good["events"],1);
        assert_eq!(good["bytes"],8193);
        assert_eq!(good["bytes_per_second"],4096.5);
        assert_eq!(good["average_payload_bytes"],8193.0);
        assert_eq!(good["framed_bytes"],8195);
        assert_eq!(good["lifetime_min_payload_bytes"],100);
        assert_eq!(good["lifetime_max_payload_bytes"],8193);
        assert_eq!(good["size_bins"]["8193-16384"],1);
        assert_eq!(good["size_bins"]["0-128"],0);
        assert_eq!(good["total_events"],2);
        let rejected = rates.iter().find(|r|r["outcome"]=="oversized").unwrap();
        assert_eq!(rejected["bytes"],0);
        assert_eq!(rejected["framed_bytes"],3);
        assert_eq!(rejected["lifetime_max_declared_payload_bytes"],20_000);
        assert_eq!(protocol_message_rates(&after, &after, 1000).len(),0);
        assert_eq!(rates.iter().find(|r|r["protocol"]=="identify").unwrap()["direction"],"encoded");
    }

    #[test]
    fn bandwidth_counts_unsampled_traffic_and_separates_layers() {
        let accounting = Accounting::default();
        for _ in 0..100_000 {
            accounting.retain("udp_socket_sent", &json!({"peer":"player","src_port":1001,
                "dst_port":1001,"wire_bytes":200,"payload_bytes":160}), 0);
        }
        accounting.retain("tunnel_queued", &json!({"peer":"player","bytes":500}), 0);
        let mut previous = Value::Null;
        let first = accounting.bandwidth(&mut previous, 2000);
        assert_eq!(first["stages"]["udp_socket_sent"]["bytes"], 20_000_000);
        assert_eq!(first["stages"]["udp_socket_sent"]["MB_per_second"], 10.0);
        assert_eq!(first["stages"]["udp_socket_sent"]["Mbps"], 80.0);
        assert_eq!(first["stages"]["tunnel_written"]["bytes"], 0);
        assert_eq!(first["flows"][0]["payload_bytes"], 16_000_000);
        assert_eq!(first["flows"][0]["max_bytes"], 200);
        let second = accounting.bandwidth(&mut previous, 1000);
        assert_eq!(second["stages"]["udp_socket_sent"]["bytes"], 0);
        assert!(second["flows"].as_array().unwrap().is_empty());
        for port in 0..400 {
            accounting.retain("udp_received", &json!({"peer":"other","src_port":port,"bytes":1}), 0);
        }
        let bounded = accounting.bandwidth(&mut previous, 1000);
        assert_eq!(bounded["flows"].as_array().unwrap().len(), 256);
        assert_eq!(bounded["flow_detail_missed_events"], 144);
        assert_eq!(bounded["stages"]["udp_received"]["events"], 400);
    }

    #[test]
    fn memory_buffer_keeps_two_minutes_with_a_hard_byte_ceiling() {
        let mut history = History { limit:MEMORY_BYTES,..History::default() };
        for n in 0..800 {
            history.push(json!({"elapsed_ms":n*200,"kind":"runtime_memory_sample","hex":"a".repeat(20_000)}));
        }
        assert_eq!(history.rows.len(),601);
        assert!(history.bytes<=MEMORY_BYTES);
        for _ in 0..400 { history.push(json!({"elapsed_ms":160_000,"kind":"runtime_memory_sample","hex":"b".repeat(60_000)})); }
        assert!(history.bytes<=MEMORY_BYTES); assert!(history.evicted>0);
        let (before,after)=split_history(&history,(160_000,0));
        assert_eq!(before.limit,MEMORY_BYTES); assert_eq!(after.limit,MEMORY_BYTES);
    }

    #[test]
    fn memory_history_survives_packet_flood_and_incident_flush() {
        let directory = test_directory();
        let (recorder, thread) = test_recorder(directory.clone(), Instant::now());
        recorder.record("runtime_memory_sample", json!({"frame":55,"blocks":[{"hex":"01020304"}]}));
        for _ in 0..100_000 { recorder.record("udp_received",json!({"bytes":1500})); }
        recorder.incident("runtime_host_crash", json!({"test":true}));
        recorder.flush();
        let data = report(&directory);
        assert_eq!(data["memory_before"][0]["fields"]["frame"], 55);
        assert!(data["decision_before"].as_array().unwrap().iter()
            .all(|row| row["kind"] != "runtime_memory_sample"));
        drop(recorder);
        thread.join().unwrap();
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn context_updates_merge_and_flush_includes_preceding_priority_rows() {
        let directory = test_directory();
        let (recorder, thread) = test_recorder(directory.clone(), Instant::now());
        recorder.context(json!({"build":"test","runtime":{"process_id":42}}));
        recorder.context(json!({"profile_name":"Player"}));
        recorder.incident("game_leave_session", json!({"reason_code":11}));
        recorder.flush();
        let data = report(&directory);
        assert_eq!(data["session"]["context"]["build"], "test");
        assert_eq!(data["session"]["context"]["runtime"]["process_id"], 42);
        assert_eq!(data["session"]["context"]["profile_name"], "Player");
        assert!(data["decision_before"]
            .as_array()
            .unwrap()
            .iter()
            .any(|r| r["kind"] == "game_leave_session"));
        drop(recorder);
        thread.join().unwrap();
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn routine_packet_flood_keeps_exact_counts_without_saturating_decisions() {
        let (tx, rx) = mpsc::sync_channel(QUEUE);
        let (urgent, urgent_rx) = mpsc::sync_channel(1024);
        let recorder = Recorder {
            tx: Some(tx),
            urgent: Some(urgent),
            dropped: Arc::new(AtomicU64::new(0)),
            polled: Arc::new(AtomicU64::new(0)),
            accounting: Arc::new(Accounting::default()),
            started: Instant::now(),
            seq: AtomicU64::new(0),
        };
        for _ in 0..100_000 {
            recorder.record("guest_submitted", json!({"bytes":64,"dst_port":1000}));
        }
        recorder.record("game_session_health", json!({"host_name":"host","phase":7}));
        recorder.incident("game_leave_session", json!({"reason_code":2}));
        let rows: Vec<_> = rx
            .try_iter()
            .chain(urgent_rx.try_iter())
            .filter_map(|m| match m {
                Message::Row(row, _) => Some(row),
                _ => None,
            })
            .collect();
        assert!(
            rows.iter().any(|r| r["kind"] == "game_session_health"),
            "a packet flood must not discard the session context"
        );
        assert!(
            rows.len() < 1000,
            "routine metadata must be bounded before it reaches the worker"
        );
        assert_eq!(recorder.dropped.load(Ordering::Relaxed), 0);
        let totals = recorder.accounting.snapshot();
        assert_eq!(totals["stages"]["guest_submitted"]["events"], 100_000);
        assert_eq!(totals["stages"]["guest_submitted"]["bytes"], 6_400_000);
        assert!(
            totals["stages"]["guest_submitted"]["sampled_out"]
                .as_u64()
                .unwrap()
                > 99_000
        );
    }

    #[test]
    fn game_decisions_survive_detailed_history_eviction() {
        let directory = test_directory();
        let (recorder, thread) = test_recorder(directory.clone(), Instant::now());
        let tx = recorder.tx.as_ref().unwrap();
        tx.send(Message::Row(json!({"kind":"game_session_health","elapsed_ms":0,"fields":{"host_name":"host","phase":7}}),false)).unwrap();
        for n in 0..6000 {
            tx.send(Message::Row(
                json!({"kind":"packet","elapsed_ms":1,"fields":{"n":n,"padding":"x".repeat(1000)}}),
                false,
            ))
            .unwrap();
        }
        let (done, finished) = mpsc::channel();
        tx.send(Message::Flush(done)).unwrap();
        finished.recv_timeout(Duration::from_secs(5)).unwrap();
        recorder.incident("game_leave_session", json!({"reason_code":2}));
        recorder.flush();
        drop(recorder);
        thread.join().unwrap();
        let data = report(&directory);
        assert!(
            data["decision_before"]
                .as_array()
                .is_some_and(|rows| rows.iter().any(|r| r["fields"]["host_name"] == "host")),
            "session decisions require their own protected history"
        );
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn incidents_preserve_the_main_log_and_reject_a_different_process() {
        let directory = test_directory();
        let source = directory.join("Log_File_For_Tera.log");
        fs::write(
            &source,
            "===== xrecomp session start test pid=42 =====\nknown-build\nFIRST_FAULT\n",
        )
        .unwrap();
        let (recorder, thread) = test_recorder(directory.clone(), Instant::now());
        recorder.context(
            json!({"build":"known-build","runtime":{"main_log_path":source,"process_id":42}}),
        );
        recorder.flush();
        recorder.incident("runtime_fault", json!({"target":9}));
        recorder.flush();
        let first = report(&directory);
        let snapshot = first["main_log_snapshot"]["file"]
            .as_str()
            .expect("the incident must identify its preserved main log");
        assert!(fs::read_to_string(directory.join(snapshot))
            .unwrap()
            .contains("FIRST_FAULT"));
        fs::write(
            &source,
            "===== xrecomp session start test pid=99 =====\nOTHER_PROCESS\n",
        )
        .unwrap();
        recorder.flush();
        drop(recorder);
        thread.join().unwrap();
        assert!(
            !fs::read_to_string(directory.join(snapshot))
                .unwrap()
                .contains("OTHER_PROCESS"),
            "a relaunch cannot overwrite another process's preserved log"
        );
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn priority_incidents_partition_same_millisecond_traffic_by_producer_sequence() {
        let mut history = History::default();
        for (seq, kind) in [(1, "before"), (3, "after"), (2, "disconnect")] {
            history.push(json!({"elapsed_ms":10,"seq":seq,"kind":kind}));
        }
        let (before, after) = split_history(&history, (10, 2));
        assert_eq!(
            ordered(&before)
                .iter()
                .map(|r| r["kind"].as_str().unwrap())
                .collect::<Vec<_>>(),
            ["before", "disconnect"]
        );
        assert_eq!(after.rows.len(), 1);
        assert_eq!(after.rows[0]["kind"], "after");
    }

    fn test_recorder(
        directory: PathBuf,
        started: Instant,
    ) -> (Arc<Recorder>, std::thread::JoinHandle<()>) {
        let (tx, rx) = mpsc::sync_channel(QUEUE);
        let (urgent, urgent_rx) = mpsc::sync_channel(1024);
        let dropped = Arc::new(AtomicU64::new(0));
        let polled = Arc::new(AtomicU64::new(0));
        let accounting = Arc::new(Accounting::default());
        let recorder = Arc::new(Recorder {
            tx: Some(tx),
            urgent: Some(urgent),
            dropped: dropped.clone(),
            polled: polled.clone(),
            accounting: accounting.clone(),
            started,
            seq: AtomicU64::new(0),
        });
        let thread = std::thread::spawn(move || {
            worker(
                directory,
                1,
                "tester".into(),
                rx,
                urgent_rx,
                dropped,
                polled,
                accounting,
                started,
            )
        });
        (recorder, thread)
    }

    fn report(directory: &Path) -> Value {
        let path = fs::read_dir(directory)
            .unwrap()
            .filter_map(Result::ok)
            .find(|e| e.path().extension().is_some_and(|ext| ext == "json"))
            .unwrap()
            .path();
        serde_json::from_str(&fs::read_to_string(path).unwrap()).unwrap()
    }

    #[test]
    fn complete_aftermath_keeps_health_history_when_packet_history_overflows() {
        let directory = test_directory();
        let (recorder, thread) =
            test_recorder(directory.clone(), Instant::now() - Duration::from_secs(119));
        let tx = recorder.tx.as_ref().unwrap();
        for n in 0..119 {
            tx.send(Message::Row(
                json!({"kind":"peer_health","elapsed_ms":n*1000,"fields":{"received":n}}),
                false,
            ))
            .unwrap();
        }
        for n in 0..6000 {
            tx.send(Message::Row(json!({"kind":"packet","elapsed_ms":119_000,"fields":{"n":n,"padding":"x".repeat(1000)}}),false)).unwrap();
        }
        let (done, finished) = mpsc::channel();
        tx.send(Message::Flush(done)).unwrap();
        finished.recv_timeout(Duration::from_secs(5)).unwrap();
        recorder.incident("test_disconnect", json!({"reason":"synthetic_test"}));
        recorder.record("after_marker", json!({}));
        // Exercise the real worker deadline, rather than pretending a flush
        // gathered the promised aftermath. Other tests run concurrently.
        std::thread::sleep(Duration::from_millis(AFTER_MS + 1000));
        drop(recorder);
        thread.join().unwrap();
        let report = report(&directory);
        assert_eq!(report["complete"], true);
        assert!(report["history_evicted"].as_u64().unwrap() > 0);
        assert!(report["health_before"]
            .as_array()
            .unwrap()
            .iter()
            .any(|r| r["elapsed_ms"] == 0));
        assert!(!report["before"]
            .as_array()
            .unwrap()
            .iter()
            .any(|r| r["elapsed_ms"] == 0));
        assert!(report["after"]
            .as_array()
            .unwrap()
            .iter()
            .any(|r| r["kind"] == "after_marker"));
        assert!(!report["health_after"].as_array().unwrap().is_empty());
        fs::remove_dir_all(directory).unwrap();
    }

    #[tokio::test]
    async fn real_udp_delivery_and_crypto_rejections_are_metadata_only() {
        use crate::crypto::LocalKeyAgreement;
        use crate::engine::Event;
        use crate::game_plane::{Emitter, GamePlane};
        use tokio::net::UdpSocket;
        use tokio::sync::Notify;
        let directory = test_directory();
        let (recorder, thread) = test_recorder(directory.clone(), Instant::now());
        let sender = Arc::new(UdpSocket::bind("127.0.0.1:0").await.unwrap());
        let receiver = Arc::new(UdpSocket::bind("127.0.0.1:0").await.unwrap());
        let a = libp2p::identity::Keypair::generate_ed25519()
            .public()
            .to_peer_id();
        let b = libp2p::identity::Keypair::generate_ed25519()
            .public()
            .to_peer_id();
        let (atx, _arx) = tokio::sync::mpsc::unbounded_channel();
        let (btx, mut brx) = tokio::sync::mpsc::unbounded_channel();
        let ap = GamePlane::new(sender.clone(), Emitter::new(atx, Arc::new(Notify::new())));
        let bp = GamePlane::new(receiver.clone(), Emitter::new(btx, Arc::new(Notify::new())));
        ap.set_recorder(recorder.clone());
        bp.set_recorder(recorder.clone());
        let ak = LocalKeyAgreement::new();
        let bk = LocalKeyAgreement::new();
        ap.install_keys(b, ak.derive(&bk.public_key()));
        bp.install_keys(a, bk.derive(&ak.public_key()));
        ap.map_endpoint(b, receiver.local_addr().unwrap());
        bp.map_endpoint(a, sender.local_addr().unwrap());
        ap.set_vip(b, 0x0a4d0002);
        bp.set_vip(a, 0x0a4d0001);
        sender.writable().await.unwrap();
        assert!(ap.try_send(0x0a4d0002, 1000, 2000, b"PRIVATE_TEST_DATAGRAM"));
        let mut frame = [0; 256];
        let (len, from) =
            tokio::time::timeout(Duration::from_secs(2), receiver.recv_from(&mut frame))
                .await
                .unwrap()
                .unwrap();
        assert!(bp.try_receive(from, &frame[..len]));
        let Some(Event::Datagram {
            data,
            src_port,
            dst_port,
            ..
        }) = brx.recv().await
        else {
            panic!("missing delivered datagram")
        };
        assert_eq!(data, b"PRIVATE_TEST_DATAGRAM");
        assert_eq!((src_port, dst_port), (1000, 2000));
        assert!(
            bp.try_receive(from, &frame[..len]),
            "replay consumed without delivery"
        );
        assert!(brx.try_recv().is_err());
        let mut tampered = ap.seal(b, 1000, 2000, b"PRIVATE_TEST_DATAGRAM").unwrap();
        tampered[9] ^= 1;
        assert!(bp.try_receive(from, &tampered));
        assert!(
            bp.open(a, &[]).is_none(),
            "truncated engine frames are classified safely"
        );
        recorder.incident("test_disconnect", json!({"reason":"synthetic_test"}));
        recorder.flush();
        drop(ap);
        drop(bp);
        drop(recorder);
        thread.join().unwrap();
        let report = report(&directory);
        let rows = report["before"].as_array().unwrap();
        assert!(rows
            .iter()
            .any(|r| r["kind"] == "udp_socket_sent" && r["fields"]["wire_counter"] == 0));
        assert!(rows
            .iter()
            .any(|r| r["kind"] == "udp_received" && r["fields"]["bytes"] == 21));
        for reason in [
            "replay_or_outside_window",
            "authentication_failed",
            "truncated",
        ] {
            assert!(rows
                .iter()
                .any(|r| r["kind"] == "udp_rejected" && r["fields"]["reason"] == reason));
        }
        assert!(!report.to_string().contains("PRIVATE_TEST_DATAGRAM"));
        fs::remove_dir_all(directory).unwrap();
    }

    fn test_directory() -> PathBuf {
        let directory = std::env::temp_dir().join(format!(
            "rexnet-recorder-test-{}-{}",
            std::process::id(),
            rand::random::<u64>()
        ));
        fs::create_dir_all(&directory).unwrap();
        directory
    }

    #[test]
    fn saturated_producer_is_nonblocking_and_exposes_its_lost_records() {
        let (tx, _rx) = mpsc::sync_channel(1);
        let (urgent, urgent_rx) = mpsc::sync_channel(1);
        let recorder = Recorder {
            tx: Some(tx),
            urgent: Some(urgent),
            dropped: Arc::new(AtomicU64::new(0)),
            polled: Arc::new(AtomicU64::new(0)),
            accounting: Arc::new(Accounting::default()),
            started: Instant::now(),
            seq: AtomicU64::new(0),
        };
        let start = Instant::now();
        for n in 0..10000 {
            recorder.record("packet", json!({"n":n}));
        }
        assert_eq!(recorder.dropped.load(Ordering::Relaxed), 9999);
        recorder.incident("disconnect", json!({"reason":"test"}));
        assert!(
            matches!(urgent_rx.try_recv(),Ok(Message::Row(row,true)) if row["kind"]=="disconnect")
        );
        assert!(start.elapsed() < Duration::from_secs(2));
    }

    #[test]
    fn a_report_survives_shutdown_and_a_second_launch_without_payload_collection() {
        let directory = test_directory();
        for _ in 0..2 {
            let (tx, rx) = mpsc::sync_channel(QUEUE);
            let (_urgent, urgent_rx) = mpsc::sync_channel(1);
            let path = directory.clone();
            let thread = std::thread::spawn(move || {
                worker(
                    path,
                    1,
                    "tester".into(),
                    rx,
                    urgent_rx,
                    Arc::new(AtomicU64::new(0)),
                    Arc::new(AtomicU64::new(0)),
                    Arc::new(Accounting::default()),
                    Instant::now(),
                )
            });
            tx.send(Message::Context(json!({"build":"known-build"})))
                .unwrap();
            tx.send(Message::Row(
                json!({"kind":"guest_queued","elapsed_ms":1,"fields":{"bytes":12}}),
                false,
            ))
            .unwrap();
            tx.send(Message::Row(
                json!({"kind":"game_leave_session","elapsed_ms":2,"fields":{"reason_code":11}}),
                true,
            ))
            .unwrap();
            tx.send(Message::Row(
                json!({"kind":"guest_consumed","elapsed_ms":3,"fields":{"bytes":12}}),
                false,
            ))
            .unwrap();
            drop(tx);
            thread.join().unwrap();
        }
        let reports: Vec<_> = fs::read_dir(&directory)
            .unwrap()
            .filter_map(Result::ok)
            .filter(|e| e.path().extension().is_some_and(|ext| ext == "json"))
            .collect();
        assert_eq!(reports.len(), 2, "each launch keeps its incident");
        for entry in reports {
            let body = fs::read_to_string(entry.path()).unwrap();
            let report: Value = serde_json::from_str(&body).unwrap();
            assert_eq!(
                report["complete"], false,
                "shutdown before the 30-second aftermath is labelled incomplete"
            );
            assert_eq!(report["session"]["context"]["build"], "known-build");
            assert_eq!(report["before"][0]["kind"], "guest_queued");
            assert_eq!(report["after"][0]["kind"], "guest_consumed");
            assert!(!body.contains("\"payload\":"));
        }
        let logs: Vec<_> = fs::read_dir(&directory)
            .unwrap()
            .filter_map(Result::ok)
            .filter(|e| e.path().extension().is_some_and(|ext| ext == "jsonl"))
            .collect();
        for log in logs {
            let body = fs::read_to_string(log.path()).unwrap();
            let first: Value = serde_json::from_str(body.lines().next().unwrap()).unwrap();
            assert_eq!(
                first["kind"], "session_start",
                "every log carries its launch identity"
            );
        }
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn pruning_keeps_foreign_files_and_only_removes_our_oldest_reports() {
        let directory = test_directory();
        fs::write(directory.join("personal.json"), "keep").unwrap();
        for n in 0..6 {
            fs::write(directory.join(format!("netdiag-{n}.json")), "{}").unwrap();
        }
        prune(&directory, "json", 2);
        assert_eq!(fs::read_dir(&directory).unwrap().count(), 3);
        assert_eq!(
            fs::read_to_string(directory.join("personal.json")).unwrap(),
            "keep"
        );
        fs::remove_dir_all(directory).unwrap();
    }

    #[test]
    fn history_keeps_two_minutes_and_reports_evictions() {
        let mut history = History::default();
        history.push(json!({"elapsed_ms": 0, "kind": "packet"}));
        history.push(json!({"elapsed_ms": 120_001, "kind": "disconnect"}));
        assert_eq!(history.rows.len(), 1);
        assert_eq!(history.rows[0]["kind"], "disconnect");
        assert_eq!(history.evicted, 1);
    }

    #[test]
    fn history_has_a_hard_byte_ceiling_even_during_a_packet_flood() {
        let mut history = History::default();
        for _ in 0..6000 {
            history.push(json!({"elapsed_ms": 1, "kind": "packet", "padding": "x".repeat(1000)}));
        }
        assert!(history.bytes <= HISTORY_BYTES);
        assert!(history.evicted > 0);
        assert!(!history.rows.is_empty());
    }
}
