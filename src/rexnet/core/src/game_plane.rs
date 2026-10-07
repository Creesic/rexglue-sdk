// @file        rexnet/core/src/game_plane.rs
// @brief       Game-plane hot path, shared off the swarm loop.
//
// @copyright   Copyright (c) 2026 Ryan Fisher <ryanfisher099@gmail.com>
//              All rights reserved.
//
// @license     BSD 3-Clause License
//              See LICENSE file in the project root for full license text.

//! The encrypted UDP hot path (spec §6), factored out of the engine so game
//! traffic never queues behind DHT, gossip or identify work.
//!
//! The engine still owns negotiation: it punches endpoints, agrees keys and
//! allocates virtual IPs, and records the results here. Once a peer is
//! *ready* (endpoint mapped and keys installed) the traffic itself bypasses
//! the engine loop in both directions: the UDP reader task decrypts inbound
//! frames and emits them straight to the shim, and the FFI send call seals
//! and transmits on the caller's thread. Anything not ready — a peer still
//! punching, a tunnelled peer, a broadcast, a probe — falls through to the
//! engine exactly as before.
//!
//! Every table here is mutated only by the engine; the fast paths take the
//! lock briefly to look up and to advance the per-peer nonce counter or
//! replay window.

use std::collections::HashMap;
use std::net::SocketAddr;
use std::sync::{Arc, Mutex, MutexGuard, OnceLock};
use std::time::Instant;

#[derive(Default, Clone, Copy)]
pub(crate) struct LinkHealth {
    pub sealed: u64,
    pub received: u64,
    pub rejected: u64,
    pub send_errors: u64,
    pub last_rx: Option<Instant>,
}

use libp2p::PeerId;
use tokio::net::UdpSocket;
use tokio::sync::{mpsc, Notify};

use crate::crypto::{self, SessionKeys};
use crate::engine::{Event, FRAME_DATA};

/// Event sink shared by the engine and the fast paths. Every emit pings the
/// notify so a blocked `rexnet_wait_event` wakes at once instead of at the
/// next pump tick.
#[derive(Clone)]
pub struct Emitter {
    tx: mpsc::UnboundedSender<Event>,
    notify: Arc<Notify>,
}

impl Emitter {
    pub fn new(tx: mpsc::UnboundedSender<Event>, notify: Arc<Notify>) -> Self {
        Self { tx, notify }
    }

    pub fn emit(&self, event: Event) {
        let _ = self.tx.send(event);
        self.notify.notify_one();
    }
}

#[derive(Default)]
struct Tables {
    keys: HashMap<PeerId, SessionKeys>,
    endpoint_by_peer: HashMap<PeerId, SocketAddr>,
    peer_by_endpoint: HashMap<SocketAddr, PeerId>,
    vip_by_peer: HashMap<PeerId, u32>,
    peer_by_vip: HashMap<u32, PeerId>,
    health: HashMap<PeerId, LinkHealth>,
}

pub struct GamePlane {
    recorder: OnceLock<Arc<crate::diagnostics::Recorder>>,
    socket: Arc<UdpSocket>,
    emitter: Emitter,
    tables: Mutex<Tables>,
}

impl GamePlane {
    pub fn new(socket: Arc<UdpSocket>, emitter: Emitter) -> Self {
        Self {
            recorder: OnceLock::new(),
            socket,
            emitter,
            tables: Mutex::new(Tables::default()),
        }
    }

    pub(crate) fn set_recorder(&self, recorder: Arc<crate::diagnostics::Recorder>) {
        let _ = self.recorder.set(recorder);
    }

    fn lock(&self) -> MutexGuard<'_, Tables> {
        // A panic while holding the lock leaves the tables consistent (every
        // mutation is a single insert/remove), so poisoning carries no signal.
        self.tables.lock().unwrap_or_else(|e| e.into_inner())
    }

    pub(crate) fn recorder(&self) -> Option<&Arc<crate::diagnostics::Recorder>> { self.recorder.get() }

    // --- Engine-side bookkeeping ------------------------------------------

    pub fn map_endpoint(&self, peer: PeerId, addr: SocketAddr) {
        let mut t = self.lock();
        if let Some(recorder) = self.recorder.get() {
            recorder.record("endpoint_mapped",serde_json::json!({"peer":peer.to_base58(),
                "endpoint":addr.to_string(),"previous":t.endpoint_by_peer.get(&peer).map(|a|a.to_string())}));
        }
        t.endpoint_by_peer.insert(peer, addr);
        t.peer_by_endpoint.insert(addr, peer);
    }

    pub fn unmap_endpoint(&self, peer: &PeerId) -> Option<SocketAddr> {
        let mut t = self.lock();
        let addr = t.endpoint_by_peer.remove(peer)?;
        t.peer_by_endpoint.remove(&addr);
        Some(addr)
    }

    pub(crate) fn health(&self, peer: &PeerId) -> LinkHealth {
        self.lock().health.get(peer).copied().unwrap_or_default()
    }

    pub fn endpoint(&self, peer: &PeerId) -> Option<SocketAddr> {
        self.lock().endpoint_by_peer.get(peer).copied()
    }

    pub fn peer_at(&self, addr: &SocketAddr) -> Option<PeerId> {
        self.lock().peer_by_endpoint.get(addr).copied()
    }

    /// False if the peer already had keys; an established session is left
    /// alone so its counter and replay window are never rewound.
    pub fn install_keys(&self, peer: PeerId, keys: SessionKeys) -> bool {
        let mut t = self.lock();
        if t.keys.contains_key(&peer) {
            if let Some(recorder) = self.recorder.get() {
                recorder.record("game_keys_retained",serde_json::json!({"peer":peer.to_base58()}));
            }
            return false;
        }
        t.keys.insert(peer, keys);
        if let Some(recorder) = self.recorder.get() {
            recorder.record("game_keys_installed",serde_json::json!({"peer":peer.to_base58()}));
        }
        true
    }

    pub fn remove_keys(&self, peer: &PeerId) {
        if self.lock().keys.remove(peer).is_some() {
            if let Some(recorder) = self.recorder.get() {
                recorder.record("game_keys_removed",serde_json::json!({"peer":peer.to_base58()}));
            }
        }
    }

    pub fn has_keys(&self, peer: &PeerId) -> bool {
        self.lock().keys.contains_key(peer)
    }

    /// Endpoint mapped and keys installed: traffic can flow both ways.
    pub fn ready(&self, peer: &PeerId) -> bool {
        let t = self.lock();
        t.endpoint_by_peer.contains_key(peer) && t.keys.contains_key(peer)
    }

    pub fn set_vip(&self, peer: PeerId, vip: u32) {
        let mut t = self.lock();
        if let Some(old) = t.vip_by_peer.insert(peer, vip) {
            if old != vip {
                t.peer_by_vip.remove(&old);
            }
        }
        t.peer_by_vip.insert(vip, peer);
    }

    pub fn clear_vip(&self, peer: &PeerId) {
        let mut t = self.lock();
        t.health.remove(peer);
        if let Some(vip) = t.vip_by_peer.remove(peer) {
            t.peer_by_vip.remove(&vip);
        }
    }

    // --- Sealing, used by the engine for queued and broadcast traffic ------

    /// `None` when there are no keys — the caller drops it. No plaintext
    /// fallback: that would be a downgrade an attacker could force.
    pub fn seal(&self, peer: PeerId, src_port: u16, dst_port: u16, data: &[u8]) -> Option<Vec<u8>> {
        let mut t = self.lock();
        let Some(keys) = t.keys.get_mut(&peer) else {
            tracing::warn!(%peer, "no game-plane keys; datagram dropped rather than sent in clear");
            if let Some(recorder) = self.recorder.get() {
                recorder.record("udp_seal_rejected", serde_json::json!({"peer":peer.to_base58(),"reason":"no_keys"}));
            }
            return None;
        };
        let frame = seal_with(keys, peer, src_port, dst_port, data);
        if frame.is_some() { t.health.entry(peer).or_default().sealed += 1; }
        frame
    }

    /// Engine sends (broadcasts and held traffic) have the same visibility as
    /// the fast path. A successful socket send is not a delivery acknowledgement.
    pub(crate) fn send_frame(&self, peer: PeerId, endpoint: SocketAddr, frame: &[u8], src_port: u16, dst_port: u16) {
        if transmit(&self.socket, self.recorder.get(), peer, endpoint, frame, src_port, dst_port).is_err() {
            self.lock().health.entry(peer).or_default().send_errors += 1;
        }
    }

    /// Authenticate and decrypt an inbound game datagram.
    pub fn open(&self, peer: PeerId, frame: &[u8]) -> Option<(u16, u16, Vec<u8>)> {
        let mut t = self.lock();
        let keys = t.keys.get_mut(&peer)?;
        let result = open_with(keys, peer, frame, self.recorder.get());
        record_receive(&mut t.health, peer, result.is_some());
        if let Some((src, dst, data)) = &result {
            if let Some(recorder) = self.recorder.get() {
                recorder.record("udp_received", serde_json::json!({"peer":peer.to_base58(),
                    "virtual_ip":t.vip_by_peer.get(&peer),"src_port":src,"dst_port":dst,"bytes":data.len(),
                    "wire_counter":wire_counter(frame)}));
            }
        }
        result
    }

    // --- Fast paths --------------------------------------------------------

    /// Seal and transmit on the caller's thread if the peer is ready. False
    /// means the caller must hand the datagram to the engine: unknown or
    /// broadcast address, punch still in flight, or a tunnelled peer.
    pub fn try_send(&self, virtual_ip: u32, src_port: u16, dst_port: u16, data: &[u8]) -> bool {
        let mut t = self.lock();
        let Some(peer) = t.peer_by_vip.get(&virtual_ip).copied() else {
            return false;
        };
        let Some(endpoint) = t.endpoint_by_peer.get(&peer).copied() else {
            return false;
        };
        let Some(keys) = t.keys.get_mut(&peer) else {
            return false;
        };
        if let Some(frame) = seal_with(keys, peer, src_port, dst_port, data) {
            // Non-blocking on purpose: the guest thinks this is UDP, and a
            // full socket buffer should drop rather than stall a game thread.
            let health = t.health.entry(peer).or_default();
            health.sealed += 1;
            if let Err(error) = transmit(&self.socket, self.recorder.get(), peer, endpoint, &frame, src_port, dst_port) {
                health.send_errors += 1;
                if health.send_errors.is_power_of_two() {
                    tracing::warn!(%peer, %endpoint, count = health.send_errors, ?error, "game UDP send failed");
                }
            }
        }
        true
    }

    /// Decrypt and deliver an inbound frame if its source is a ready peer.
    /// False means the engine must take it: a probe, or a data frame from an
    /// endpoint that has not finished punching (held there, not dropped).
    pub fn try_receive(&self, addr: SocketAddr, frame: &[u8]) -> bool {
        if frame.first() != Some(&FRAME_DATA) || frame.len() < 1 + crypto::CRYPTO_OVERHEAD {
            return false;
        }
        let event = {
            let mut t = self.lock();
            let Some(peer) = t.peer_by_endpoint.get(&addr).copied() else {
                return false;
            };
            let Some(vip) = t.vip_by_peer.get(&peer).copied() else {
                return false;
            };
            let Some(keys) = t.keys.get_mut(&peer) else {
                return false;
            };
            // Authentic-but-rejected frames (replays, short records) are
            // consumed here: the engine could do nothing more with them.
            let result = open_with(keys, peer, frame, self.recorder.get());
            record_receive(&mut t.health, peer, result.is_some());
            if let Some(recorder) = self.recorder.get() {
                match &result {
                    Some((src, dst, data)) => { recorder.record("udp_received",serde_json::json!({
                        "peer":peer.to_base58(),"virtual_ip":vip,"src_port":src,"dst_port":dst,
                        "bytes":data.len(),"wire_counter":u64::from_be_bytes(frame[1..9].try_into().unwrap())})); }
                    None => {}
                }
            }
            result.map(|(src_port, dst_port, data)| Event::Datagram {
                virtual_ip: vip,
                src_port,
                dst_port,
                data,
            })
        };
        if let Some(event) = event {
            self.emitter.emit(event);
        }
        true
    }
}

fn record_receive(health: &mut HashMap<PeerId, LinkHealth>, peer: PeerId, valid: bool) {
    let health = health.entry(peer).or_default();
    if valid {
        health.received += 1;
        health.last_rx = Some(Instant::now());
    } else {
        health.rejected += 1;
    }
}

fn seal_with(
    keys: &mut SessionKeys,
    peer: PeerId,
    src_port: u16,
    dst_port: u16,
    data: &[u8],
) -> Option<Vec<u8>> {
    let mut plaintext = Vec::with_capacity(4 + data.len());
    plaintext.extend_from_slice(&src_port.to_be_bytes());
    plaintext.extend_from_slice(&dst_port.to_be_bytes());
    plaintext.extend_from_slice(data);
    match keys.sealer.seal(&[FRAME_DATA], &plaintext) {
        Ok(record) => {
            let mut frame = Vec::with_capacity(1 + record.len());
            frame.push(FRAME_DATA);
            frame.extend_from_slice(&record);
            Some(frame)
        }
        Err(err) => {
            tracing::warn!(%peer, ?err, "sealing a game datagram failed; dropped");
            None
        }
    }
}

fn wire_counter(frame: &[u8]) -> Option<u64> {
    frame.get(1..9).map(|bytes|u64::from_be_bytes(bytes.try_into().unwrap()))
}

fn transmit(socket: &UdpSocket, recorder: Option<&Arc<crate::diagnostics::Recorder>>,
    peer: PeerId, endpoint: SocketAddr, frame: &[u8], src_port: u16, dst_port: u16) -> std::io::Result<usize> {
    let result = socket.try_send_to(frame, endpoint);
    if let Some(recorder) = recorder {
        recorder.record(if result.is_ok() {"udp_socket_sent"} else {"udp_send_error"},
            serde_json::json!({"peer":peer.to_base58(),"endpoint":endpoint.to_string(),
                "wire_bytes":result.as_ref().copied().unwrap_or(frame.len()),
                "payload_bytes":frame.len().saturating_sub(1+4+crypto::CRYPTO_OVERHEAD),
                "src_port":src_port,"dst_port":dst_port,"wire_counter":wire_counter(frame),
                "error":result.as_ref().err().map(|e|format!("{e:?}"))}));
    }
    result
}

fn open_with(keys: &mut SessionKeys, peer: PeerId, frame: &[u8],
    recorder: Option<&Arc<crate::diagnostics::Recorder>>) -> Option<(u16, u16, Vec<u8>)> {
    let result = keys.opener.open(&[FRAME_DATA], frame.get(1..).unwrap_or_default());
    let reason = match &result {
        Ok(plain) if plain.len() >= 4 => None,
        Ok(_) => Some("short_ports"),
        Err(crypto::CryptoError::Replay) => Some("replay_or_outside_window"),
        Err(crypto::CryptoError::NotAuthentic) => Some("authentication_failed"),
        Err(crypto::CryptoError::Truncated) => Some("truncated"),
        Err(crypto::CryptoError::TooLarge) => Some("too_large"),
    };
    if let (Some(recorder), Some(reason)) = (recorder, reason) {
        recorder.record("udp_rejected",serde_json::json!({"peer":peer.to_base58(),"reason":reason,
            "wire_bytes":frame.len(),"wire_counter":wire_counter(frame)}));
    }
    match result {
        Ok(plain) if plain.len() >= 4 => Some((
            u16::from_be_bytes([plain[0], plain[1]]),
            u16::from_be_bytes([plain[2], plain[3]]),
            plain[4..].to_vec(),
        )),
        Ok(_) => {
            tracing::debug!(%peer, "authentic game datagram was too short to carry ports");
            None
        }
        Err(crypto::CryptoError::Replay) => {
            // Expected with retransmitting titles; not an attack signal.
            tracing::trace!(%peer, "replayed game datagram dropped");
            None
        }
        Err(err) => {
            tracing::debug!(%peer, ?err, "game datagram failed authentication");
            None
        }
    }
}

#[cfg(test)]
mod health_tests {
    use super::*;
    #[test]
    fn rejected_packets_do_not_hide_receive_silence() {
        let peer = libp2p::identity::Keypair::generate_ed25519().public().to_peer_id();
        let mut health = HashMap::new();
        record_receive(&mut health, peer, false);
        assert_eq!(health[&peer].rejected, 1);
        assert!(health[&peer].last_rx.is_none());
        record_receive(&mut health, peer, true);
        let last_rx = health[&peer].last_rx;
        assert_eq!(health[&peer].received, 1);
        record_receive(&mut health, peer, false);
        assert_eq!(health[&peer].last_rx, last_rx);
        assert_eq!(health[&peer].rejected, 2);
    }
}
