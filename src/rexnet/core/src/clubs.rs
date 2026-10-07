// @file        rexnet/core/src/clubs.rs
// @brief       Clubs this player owns or belongs to.

//! Each club this player owns or belongs to (MC3 Live step 3). The record is
//! the title's and opaque here; rexnet keeps who owns it, its version and its
//! member peers, so it can answer queries, push updates and find members.
//! Saved as postcard in `clubs.bin` through a temporary file and a rename. A
//! deleted club is remembered for 30 days so members who were away learn of
//! it; a provisional club (version 0: joined, not yet confirmed by the owner)
//! is dropped after 30 days without a record.

use std::collections::HashSet;
use std::path::{Path, PathBuf};

use libp2p::PeerId;
use serde::{Deserialize, Serialize};

/// Largest club record carried.
pub const MAX_RECORD: usize = 64 * 1024;
const KEEP_SECS: u64 = 30 * 24 * 3600;
/// More member peers than any title's roster holds (the XDK caps teams at 100).
pub const MAX_MEMBERS_CARRIED: usize = 256;

/// What a club entry may hold: anything bigger came from a broken or
/// hostile peer, and would block the FFI's take queue.
fn carried(record: &[u8], members: &[PeerId]) -> bool {
    record.len() <= MAX_RECORD && members.len() <= MAX_MEMBERS_CARRIED
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, Serialize, Deserialize)]
pub enum Role {
    Owner,
    Member,
    Deleted,
}

#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
pub struct Club {
    pub team: u64,
    owner: Vec<u8>,
    pub version: u64,
    pub record: Vec<u8>,
    members: Vec<Vec<u8>>,
    /// The DHT name key this owner advertises.
    pub name_key: Option<String>,
    pub role: Role,
    /// When the role last changed (unix seconds).
    pub since: u64,
}

impl Club {
    pub fn owner(&self) -> Option<PeerId> {
        PeerId::from_bytes(&self.owner).ok()
    }

    pub fn members(&self) -> Vec<PeerId> {
        self.members.iter().filter_map(|m| PeerId::from_bytes(m).ok()).collect()
    }

    pub fn has_member(&self, peer: &PeerId) -> bool {
        self.members.contains(&peer.to_bytes())
    }
}

#[derive(Default, Deserialize)]
struct File {
    #[allow(dead_code)]
    schema: u8,
    clubs: Vec<Club>,
}

#[derive(Serialize)]
struct FileRef<'a> {
    schema: u8,
    clubs: &'a [Club],
}

pub struct ClubStore {
    path: PathBuf,
    clubs: Vec<Club>,
}

fn peer_bytes(peers: &[PeerId]) -> Vec<Vec<u8>> {
    peers.iter().map(|p| p.to_bytes()).collect()
}

impl ClubStore {
    pub fn load(data_dir: &Path) -> Self {
        let path = data_dir.join("clubs.bin");
        // Only a missing file means "none": anything else is set aside so the
        // next save cannot overwrite clubs we failed to read.
        let file = match std::fs::read(&path) {
            Ok(bytes) => postcard::from_bytes::<File>(&bytes).map_err(|err| err.to_string()),
            Err(err) if err.kind() == std::io::ErrorKind::NotFound => Ok(File::default()),
            Err(err) => Err(err.to_string()),
        }
        .unwrap_or_else(|err| {
            tracing::warn!(%err, "clubs.bin unreadable; set aside, starting empty");
            let _ = std::fs::rename(&path, path.with_extension("bin.bad"));
            File::default()
        });
        Self { path, clubs: file.clubs }
    }

    fn save(&self) {
        let file = FileRef { schema: 1, clubs: &self.clubs };
        let bytes = match postcard::to_allocvec(&file) {
            Ok(bytes) => bytes,
            Err(err) => return tracing::warn!(%err, "clubs not saved"),
        };
        if let Some(parent) = self.path.parent() {
            let _ = std::fs::create_dir_all(parent);
        }
        let tmp = self.path.with_extension("bin.tmp");
        if let Err(err) = std::fs::write(&tmp, bytes).and_then(|_| std::fs::rename(&tmp, &self.path)) {
            tracing::warn!(%err, "clubs not saved");
        }
    }

    pub fn get(&self, team: u64) -> Option<&Club> {
        self.clubs.iter().find(|c| c.team == team)
    }

    pub fn clubs(&self) -> &[Club] {
        &self.clubs
    }

    fn put(&mut self, club: Club) {
        self.clubs.retain(|c| c.team != club.team);
        self.clubs.push(club);
        self.save();
    }

    /// We own `team` at `version`: our own publish, or a handover we received.
    #[allow(clippy::too_many_arguments)]
    pub fn set_owned(
        &mut self,
        team: u64,
        local: &PeerId,
        version: u64,
        record: Vec<u8>,
        members: &[PeerId],
        name_key: Option<String>,
        now: u64,
    ) -> bool {
        if !carried(&record, members) {
            tracing::info!(team, "club record over the limits refused");
            return false;
        }
        let owner = local.to_bytes();
        self.put(Club { team, owner, version, record, members: peer_bytes(members), name_key, role: Role::Owner, since: now });
        true
    }

    /// The owner's record of a club we belong to. False, and unchanged,
    /// unless it is newer than ours.
    pub fn set_member(&mut self, team: u64, owner: &PeerId, version: u64, record: Vec<u8>, members: &[PeerId], now: u64) -> bool {
        if !carried(&record, members) {
            tracing::info!(team, "club record over the limits refused");
            return false;
        }
        if self.get(team).is_some_and(|c| c.role != Role::Deleted && c.version >= version) {
            return false;
        }
        let owner = owner.to_bytes();
        self.put(Club { team, owner, version, record, members: peer_bytes(members), name_key: None, role: Role::Member, since: now });
        true
    }

    /// Joined `team`, not yet confirmed by its owner: version 0 with a
    /// provisional record. A confirmed copy is kept.
    pub fn track(&mut self, team: u64, owner: &PeerId, record: Vec<u8>, now: u64) {
        if self.get(team).is_some_and(|c| c.role == Role::Member && c.version > 0) {
            return;
        }
        let owner = owner.to_bytes();
        self.put(Club { team, owner, version: 0, record, members: Vec::new(), name_key: None, role: Role::Member, since: now });
    }

    /// The club's owner is someone else now.
    pub fn set_owner(&mut self, team: u64, owner: &PeerId) {
        if let Some(club) = self.clubs.iter_mut().find(|c| c.team == team) {
            club.owner = owner.to_bytes();
            self.save();
        }
    }

    /// We deleted `team`: keep a marker so members who were away learn of it.
    pub fn mark_deleted(&mut self, team: u64, now: u64) {
        if let Some(club) = self.clubs.iter_mut().find(|c| c.team == team) {
            club.role = Role::Deleted;
            club.record.clear();
            club.name_key = None;
            club.since = now;
            self.save();
        }
    }

    pub fn forget(&mut self, team: u64) {
        let before = self.clubs.len();
        self.clubs.retain(|c| c.team != team);
        if self.clubs.len() != before {
            self.save();
        }
    }

    /// Everyone we share a live club with, owners included, never us.
    pub fn peers(&self, local: &PeerId) -> Vec<PeerId> {
        let peers: HashSet<PeerId> = self
            .clubs
            .iter()
            .filter(|c| c.role != Role::Deleted)
            .flat_map(|c| c.owner().into_iter().chain(c.members()))
            .filter(|p| p != local)
            .collect();
        peers.into_iter().collect()
    }

    pub fn knows(&self, peer: &PeerId, local: &PeerId) -> bool {
        peer != local && self.peers(local).contains(peer)
    }

    /// Drop deleted markers and still-provisional clubs after 30 days;
    /// returns the provisional teams dropped.
    pub fn expire(&mut self, now: u64) -> Vec<u64> {
        let old = |c: &Club| now.saturating_sub(c.since) >= KEEP_SECS;
        let provisional = |c: &Club| c.role == Role::Member && c.version == 0;
        let dropped: Vec<u64> = self.clubs.iter().filter(|c| provisional(c) && old(c)).map(|c| c.team).collect();
        let before = self.clubs.len();
        self.clubs.retain(|c| !(old(c) && (c.role == Role::Deleted || provisional(c))));
        if self.clubs.len() != before {
            self.save();
        }
        dropped
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::collections::HashSet;

    fn temp_dir(name: &str) -> PathBuf {
        let dir = std::env::temp_dir()
            .join(format!("rexnet-clubs-{name}-{}-{}", std::process::id(), rand::random::<u64>()));
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    fn peer() -> PeerId {
        libp2p::identity::Keypair::generate_ed25519().public().to_peer_id()
    }

    const DAYS_30: u64 = 30 * 24 * 3600;

    #[test]
    fn clubs_survive_a_restart_and_list_their_peers() {
        let dir = temp_dir("restart");
        let (me, owner, m1, m2) = (peer(), peer(), peer(), peer());
        let mut store = ClubStore::load(&dir);
        store.set_owned(1, &me, 3, b"mine".to_vec(), &[m1, m2], Some("key".into()), 100);
        assert!(store.set_member(2, &owner, 5, b"theirs".to_vec(), &[me, m1], 100));
        store.track(3, &owner, b"provisional".to_vec(), 100);
        let store = ClubStore::load(&dir);
        let owned = store.get(1).unwrap();
        assert_eq!(
            (owned.role, owned.version, owned.record.as_slice(), owned.name_key.as_deref(), owned.owner()),
            (Role::Owner, 3, &b"mine"[..], Some("key"), Some(me))
        );
        assert_eq!(store.get(2).unwrap().members(), vec![me, m1]);
        assert_eq!((store.get(3).unwrap().version, store.get(3).unwrap().owner()), (0, Some(owner)));
        let peers: HashSet<PeerId> = store.peers(&me).into_iter().collect();
        assert_eq!(peers, HashSet::from([owner, m1, m2]));
        assert!(store.knows(&m2, &me) && !store.knows(&peer(), &me));
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn only_a_newer_record_replaces_a_members_copy() {
        let dir = temp_dir("newer");
        let (me, owner) = (peer(), peer());
        let mut store = ClubStore::load(&dir);
        assert!(store.set_member(1, &owner, 2, b"two".to_vec(), &[me], 0));
        assert!(!store.set_member(1, &owner, 2, b"again".to_vec(), &[me], 0));
        assert!(!store.set_member(1, &owner, 1, b"older".to_vec(), &[me], 0));
        assert!(store.set_member(1, &owner, 3, b"three".to_vec(), &[me], 0));
        assert_eq!(store.get(1).unwrap().record, b"three");
        // A provisional club takes the owner's first record; tracking again
        // does not undo a confirmed copy.
        store.track(2, &owner, b"prov".to_vec(), 0);
        assert!(store.set_member(2, &owner, 1, b"real".to_vec(), &[me], 0));
        store.track(2, &owner, b"prov again".to_vec(), 0);
        assert_eq!((store.get(2).unwrap().version, store.get(2).unwrap().record.as_slice()), (1, &b"real"[..]));
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn deleted_markers_and_unconfirmed_clubs_expire_after_30_days() {
        let dir = temp_dir("expire");
        let (me, owner, m) = (peer(), peer(), peer());
        let mut store = ClubStore::load(&dir);
        store.set_owned(1, &me, 1, b"r".to_vec(), &[m], Some("key".into()), 0);
        store.mark_deleted(1, 1_000);
        let deleted = store.get(1).unwrap();
        assert_eq!((deleted.role, deleted.record.len(), deleted.name_key.clone()), (Role::Deleted, 0, None));
        assert!(!store.knows(&m, &me), "a deleted club's members are not our peers");
        store.track(2, &owner, b"p".to_vec(), 1_000);
        store.set_member(3, &owner, 4, b"r".to_vec(), &[me], 1_000);
        assert!(store.expire(1_000 + DAYS_30 - 1).is_empty());
        assert_eq!(store.clubs().len(), 3);
        assert_eq!(store.expire(1_000 + DAYS_30), vec![2]);
        assert!(store.get(1).is_none() && store.get(2).is_none() && store.get(3).is_some());
        assert_eq!(ClubStore::load(&dir).clubs().len(), 1);
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn a_store_that_cannot_be_read_is_set_aside_not_overwritten() {
        let dir = temp_dir("locked");
        // A directory in its place: reading fails with something other than NotFound.
        std::fs::create_dir_all(dir.join("clubs.bin")).unwrap();
        std::fs::write(dir.join("clubs.bin").join("keep"), b"x").unwrap();
        let mut store = ClubStore::load(&dir);
        store.set_owned(1, &peer(), 1, b"r".to_vec(), &[], None, 0);
        assert!(dir.join("clubs.bin.bad").join("keep").exists());
        let garbage = temp_dir("garbage");
        std::fs::write(garbage.join("clubs.bin"), [0xFF; 12]).unwrap();
        assert!(ClubStore::load(&garbage).clubs().is_empty());
        assert!(garbage.join("clubs.bin.bad").exists());
        std::fs::remove_dir_all(dir).unwrap();
        std::fs::remove_dir_all(garbage).unwrap();
    }


    #[test]
    fn records_too_big_or_with_too_many_members_are_refused() {
        let dir = temp_dir("limits");
        let (me, owner) = (peer(), peer());
        let mut store = ClubStore::load(&dir);
        assert!(!store.set_member(1, &owner, 1, vec![0; MAX_RECORD + 1], &[me], 0));
        let crowd: Vec<PeerId> = (0..MAX_MEMBERS_CARRIED + 1).map(|_| peer()).collect();
        assert!(!store.set_member(1, &owner, 1, b"r".to_vec(), &crowd, 0));
        assert!(!store.set_owned(2, &me, 1, vec![0; MAX_RECORD + 1], &[], None, 0));
        assert!(store.get(1).is_none() && store.get(2).is_none());
        std::fs::remove_dir_all(dir).unwrap();
    }
}
