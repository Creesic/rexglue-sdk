//! Size metadata only. Observers cannot inspect packet contents.
use std::sync::OnceLock;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Direction {
    Encoded,
    Received,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Outcome {
    Complete,
    Oversized,
    Malformed,
    Incomplete,
}

#[derive(Clone, Copy, Debug)]
pub struct Observation {
    pub message_type: &'static str,
    pub direction: Direction,
    pub outcome: Outcome,
    pub payload_bytes: usize,
    pub prefix_bytes: usize,
    pub declared_payload_bytes: Option<usize>,
    pub dht_rpc: Option<u32>,
}

static OBSERVER: OnceLock<fn(Observation)> = OnceLock::new();

/// Register one process-wide, non-panicking callback. No callback means no observation work.
pub fn install_observer(observer: fn(Observation)) -> bool {
    OBSERVER.set(observer).is_ok()
}

pub(crate) fn observe<T>(
    direction: Direction,
    outcome: Outcome,
    payload: &[u8],
    prefix_bytes: usize,
    declared_payload_bytes: Option<usize>,
) {
    let Some(observer) = OBSERVER.get() else {
        return;
    };
    let message_type = std::any::type_name::<T>();
    // DHT opcode is protocol metadata, never a record key or value. Field order
    // is not guaranteed, so use the existing bounded Protobuf reader to find it.
    let dht_rpc = if message_type.starts_with("libp2p_kad::") {
        let mut reader = quick_protobuf::BytesReader::from_bytes(payload);
        let mut opcode = None;
        while !reader.is_eof() {
            let Ok(tag) = reader.next_tag(payload) else {
                break;
            };
            if tag == 8 {
                opcode = reader.read_uint32(payload).ok();
                break;
            }
            if reader.read_unknown(payload, tag).is_err() {
                break;
            }
        }
        opcode
    } else {
        None
    };
    observer(Observation {
        message_type,
        direction,
        outcome,
        payload_bytes: payload.len(),
        prefix_bytes,
        declared_payload_bytes,
        dht_rpc,
    });
}
