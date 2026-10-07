// @file        rexnet/core/src/mailbox.rs
// @brief       Messages waiting for delivery, and message ids already seen.

//! Store-and-forward messages. The sender keeps each message in its outbox
//! until the recipient acknowledges it; the receiver remembers the ids it has
//! seen, so a resend after a lost acknowledgement is dropped. Both expire
//! after 30 days. Saved as postcard in `mailbox.bin`, written to a temporary
//! file and renamed so a crash cannot leave half a file.

use std::collections::HashSet;
use std::path::{Path, PathBuf};
use std::time::{SystemTime, UNIX_EPOCH};

use libp2p::PeerId;
use serde::{Deserialize, Serialize};

/// Largest message blob carried.
pub const MAX_BLOB: usize = 64 * 1024;
/// Undelivered messages and seen ids are kept this long.
pub const EXPIRY_SECS: u64 = 30 * 24 * 3600;
const MAX_PENDING: usize = 500;
const MAX_PENDING_BYTES: usize = 8 * 1024 * 1024;

pub fn unix_now() -> u64 {
    SystemTime::now().duration_since(UNIX_EPOCH).map(|d| d.as_secs()).unwrap_or(0)
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
struct Pending {
    peer: Vec<u8>,
    id: [u8; 16],
    /// The envelope kind: 0 title message, 1 club request, 2 club handover.
    kind: u8,
    blob: Vec<u8>,
    queued_unix: u64,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
struct Seen {
    id: [u8; 16],
    at_unix: u64,
}

#[derive(Default, Deserialize)]
struct File {
    #[allow(dead_code)]
    schema: u8,
    outbox: Vec<Pending>,
    seen: Vec<Seen>,
}

/// `mailbox.bin` schema 1, from before envelopes had a kind.
#[derive(Deserialize)]
struct PendingV1 {
    peer: Vec<u8>,
    id: [u8; 16],
    blob: Vec<u8>,
    queued_unix: u64,
}

#[derive(Deserialize)]
struct FileV1 {
    #[allow(dead_code)]
    schema: u8,
    outbox: Vec<PendingV1>,
    seen: Vec<Seen>,
}

impl FileV1 {
    fn upgrade(self) -> File {
        let outbox = self
            .outbox
            .into_iter()
            .map(|p| Pending { peer: p.peer, id: p.id, kind: 0, blob: p.blob, queued_unix: p.queued_unix })
            .collect();
        File { schema: 2, outbox, seen: self.seen }
    }
}

#[derive(Serialize)]
struct FileRef<'a> {
    schema: u8,
    outbox: &'a [Pending],
    seen: &'a [Seen],
}

pub struct Mailbox {
    path: PathBuf,
    outbox: Vec<Pending>,
    seen: Vec<Seen>,
}

impl Mailbox {
    pub fn load(data_dir: &Path) -> Self {
        let path = data_dir.join("mailbox.bin");
        // Only a missing file means "empty": anything else is set aside so the
        // next save cannot overwrite messages we failed to read.
        let file = match std::fs::read(&path) {
            Ok(bytes) if bytes.first() == Some(&1) => {
                postcard::from_bytes::<FileV1>(&bytes).map(FileV1::upgrade).map_err(|err| err.to_string())
            }
            Ok(bytes) => postcard::from_bytes::<File>(&bytes).map_err(|err| err.to_string()),
            Err(err) if err.kind() == std::io::ErrorKind::NotFound => Ok(File::default()),
            Err(err) => Err(err.to_string()),
        }
        .unwrap_or_else(|err| {
            tracing::warn!(%err, "mailbox.bin unreadable; set aside, starting empty");
            let _ = std::fs::rename(&path, path.with_extension("bin.bad"));
            File::default()
        });
        Self { path, outbox: file.outbox, seen: file.seen }
    }

    fn save(&self) {
        let file = FileRef { schema: 2, outbox: &self.outbox, seen: &self.seen };
        let bytes = match postcard::to_allocvec(&file) {
            Ok(bytes) => bytes,
            Err(err) => return tracing::warn!(%err, "mailbox not saved"),
        };
        if let Some(parent) = self.path.parent() {
            let _ = std::fs::create_dir_all(parent);
        }
        let tmp = self.path.with_extension("bin.tmp");
        if let Err(err) = std::fs::write(&tmp, bytes).and_then(|_| std::fs::rename(&tmp, &self.path)) {
            tracing::warn!(%err, "mailbox not saved");
        }
    }

    /// Queue a title message for `peer`. Past 500 messages or 8 MB the oldest go.
    pub fn push(&mut self, peer: PeerId, id: [u8; 16], blob: Vec<u8>, now: u64) {
        self.push_kind(peer, id, 0, blob, now);
    }

    /// Queue an envelope of `kind` for `peer`.
    pub fn push_kind(&mut self, peer: PeerId, id: [u8; 16], kind: u8, blob: Vec<u8>, now: u64) {
        self.outbox.push(Pending { peer: peer.to_bytes(), id, kind, blob, queued_unix: now });
        while self.outbox.len() > MAX_PENDING
            || self.outbox.iter().map(|p| p.blob.len()).sum::<usize>() > MAX_PENDING_BYTES
        {
            let dropped = self.outbox.remove(0);
            tracing::warn!(id = ?dropped.id, "outbox full; oldest message dropped");
        }
        self.save();
    }

    /// Messages waiting for `peer`, oldest first.
    pub fn pending_for(&self, peer: &PeerId) -> Vec<([u8; 16], Vec<u8>)> {
        let key = peer.to_bytes();
        self.outbox.iter().filter(|p| p.peer == key).map(|p| (p.id, p.blob.clone())).collect()
    }

    /// Envelopes waiting for `peer`, oldest first, with their kinds.
    pub fn pending_kinds_for(&self, peer: &PeerId) -> Vec<([u8; 16], u8, Vec<u8>)> {
        let key = peer.to_bytes();
        self.outbox.iter().filter(|p| p.peer == key).map(|p| (p.id, p.kind, p.blob.clone())).collect()
    }

    /// Peers with messages waiting.
    pub fn peers(&self) -> Vec<PeerId> {
        let peers: HashSet<PeerId> = self.outbox.iter().filter_map(|p| PeerId::from_bytes(&p.peer).ok()).collect();
        peers.into_iter().collect()
    }

    /// `peer` acknowledged `id`: forget it.
    pub fn delivered(&mut self, peer: &PeerId, id: [u8; 16]) {
        let key = peer.to_bytes();
        let before = self.outbox.len();
        self.outbox.retain(|p| !(p.peer == key && p.id == id));
        if self.outbox.len() != before {
            self.save();
        }
    }

    /// Record a received id; false when it was seen already (a resend).
    pub fn first_sight(&mut self, id: [u8; 16], now: u64) -> bool {
        if self.seen.iter().any(|s| s.id == id) {
            return false;
        }
        self.seen.push(Seen { id, at_unix: now });
        self.save();
        true
    }

    /// Drop outbox entries and seen ids older than 30 days.
    pub fn expire(&mut self, now: u64) {
        let fresh = |at: u64| now.saturating_sub(at) < EXPIRY_SECS;
        let before = (self.outbox.len(), self.seen.len());
        self.outbox.retain(|p| {
            let keep = fresh(p.queued_unix);
            if !keep {
                tracing::info!(id = ?p.id, "undelivered message expired");
            }
            keep
        });
        self.seen.retain(|s| fresh(s.at_unix));
        if (self.outbox.len(), self.seen.len()) != before {
            self.save();
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn temp_dir(name: &str) -> PathBuf {
        let dir = std::env::temp_dir()
            .join(format!("rexnet-mailbox-{name}-{}-{}", std::process::id(), rand::random::<u64>()));
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    fn peer() -> PeerId {
        libp2p::identity::Keypair::generate_ed25519().public().to_peer_id()
    }

    #[test]
    fn pending_messages_and_seen_ids_survive_a_restart() {
        let dir = temp_dir("restart");
        let (p1, p2) = (peer(), peer());
        let mut mailbox = Mailbox::load(&dir);
        mailbox.push(p1, [1; 16], b"one".to_vec(), 100);
        mailbox.push(p1, [2; 16], b"two".to_vec(), 100);
        mailbox.push(p2, [3; 16], b"three".to_vec(), 100);
        assert!(mailbox.first_sight([9; 16], 100));
        let mut mailbox = Mailbox::load(&dir);
        assert_eq!(mailbox.pending_for(&p1), vec![([1; 16], b"one".to_vec()), ([2; 16], b"two".to_vec())]);
        let peers: std::collections::HashSet<PeerId> = mailbox.peers().into_iter().collect();
        assert_eq!(peers, std::collections::HashSet::from([p1, p2]));
        assert!(!mailbox.first_sight([9; 16], 101), "a resend is not new");
        mailbox.delivered(&p1, [1; 16]);
        assert_eq!(Mailbox::load(&dir).pending_for(&p1), vec![([2; 16], b"two".to_vec())]);
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn thirty_days_expire_the_outbox_and_the_seen_ids() {
        let dir = temp_dir("expire");
        let p = peer();
        let mut mailbox = Mailbox::load(&dir);
        mailbox.push(p, [1; 16], b"x".to_vec(), 1_000);
        assert!(mailbox.first_sight([2; 16], 1_000));
        mailbox.expire(1_000 + EXPIRY_SECS - 1);
        assert_eq!(mailbox.pending_for(&p).len(), 1);
        mailbox.expire(1_000 + EXPIRY_SECS);
        assert!(mailbox.pending_for(&p).is_empty());
        assert!(mailbox.first_sight([2; 16], 1_000 + EXPIRY_SECS), "a forgotten id is new again");
        assert!(Mailbox::load(&dir).pending_for(&p).is_empty());
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn the_outbox_drops_its_oldest_past_500_messages_or_8_mb() {
        let dir = temp_dir("cap");
        let p = peer();
        let mut mailbox = Mailbox::load(&dir);
        for i in 0..501u16 {
            let mut id = [0u8; 16];
            id[..2].copy_from_slice(&i.to_le_bytes());
            mailbox.push(p, id, vec![0], 1);
        }
        let pending = mailbox.pending_for(&p);
        assert_eq!(pending.len(), 500);
        assert_eq!(&pending[0].0[..2], &1u16.to_le_bytes(), "message 0 was the oldest");
        let bytes_dir = temp_dir("bytes");
        let mut mailbox = Mailbox::load(&bytes_dir);
        for i in 0..9u8 {
            mailbox.push(p, [i; 16], vec![0; 1024 * 1024], 1);
        }
        assert_eq!(mailbox.pending_for(&p).len(), 8);
        std::fs::remove_dir_all(dir).unwrap();
        std::fs::remove_dir_all(bytes_dir).unwrap();
    }

    #[test]
    fn an_unreadable_mailbox_is_set_aside() {
        let dir = temp_dir("bad");
        std::fs::write(dir.join("mailbox.bin"), [0xFF; 12]).unwrap();
        let mailbox = Mailbox::load(&dir);
        assert!(mailbox.peers().is_empty());
        assert!(dir.join("mailbox.bin.bad").exists());
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn a_mailbox_that_cannot_be_read_is_set_aside_not_overwritten() {
        let dir = temp_dir("locked");
        // A directory in its place: reading fails with something other than NotFound.
        std::fs::create_dir_all(dir.join("mailbox.bin")).unwrap();
        std::fs::write(dir.join("mailbox.bin").join("keep"), b"x").unwrap();
        let mut mailbox = Mailbox::load(&dir);
        mailbox.push(peer(), [1; 16], b"new".to_vec(), 1);
        assert!(dir.join("mailbox.bin.bad").join("keep").exists());
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn a_version_1_mailbox_loads_with_its_messages_as_title_messages() {
        #[derive(Serialize)]
        struct OldPending {
            peer: Vec<u8>,
            id: [u8; 16],
            blob: Vec<u8>,
            queued_unix: u64,
        }
        #[derive(Serialize)]
        struct OldFile {
            schema: u8,
            outbox: Vec<OldPending>,
            seen: Vec<Seen>,
        }
        let dir = temp_dir("v1");
        let p = peer();
        let old = OldFile {
            schema: 1,
            outbox: vec![OldPending { peer: p.to_bytes(), id: [4; 16], blob: b"old".to_vec(), queued_unix: 5 }],
            seen: vec![Seen { id: [6; 16], at_unix: 5 }],
        };
        std::fs::write(dir.join("mailbox.bin"), postcard::to_allocvec(&old).unwrap()).unwrap();
        let mut mailbox = Mailbox::load(&dir);
        assert_eq!(mailbox.pending_kinds_for(&p), vec![([4; 16], 0, b"old".to_vec())]);
        assert!(!mailbox.first_sight([6; 16], 6), "seen ids survive the upgrade");
        mailbox.push_kind(p, [5; 16], 1, b"club".to_vec(), 7);
        assert_eq!(
            Mailbox::load(&dir).pending_kinds_for(&p),
            vec![([4; 16], 0, b"old".to_vec()), ([5; 16], 1, b"club".to_vec())]
        );
        std::fs::remove_dir_all(dir).unwrap();
    }
}
