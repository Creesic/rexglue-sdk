// @file        rexnet/core/src/friends.rs
// @brief       Local friend list (design spec §8.2).
//
// @copyright   Copyright (c) 2026 Ryan Fisher <ryanfisher099@gmail.com>
//              All rights reserved.
//
// @license     BSD 3-Clause License
//              See LICENSE file in the project root for full license text.


//! Local friend list (design spec §8.2).
//!
//! Friendship is mutual-consent: a request/accept exchange is required
//! before presence flows in either direction, enforced at the protocol
//! layer (engine), not UI. The list is stored locally only, in
//! `friends.txt`, one peer per line:
//!
//! ```text
//! <peer-id> <kind> <name>\t<note>
//! ```
//!
//! `kind` is `friend`, `out` (we asked), `in` (they asked) or `blocked`.
//! A line holding only a peer id is a friend: the original format.

use std::collections::HashMap;
use std::path::{Path, PathBuf};
use std::str::FromStr;

use libp2p::PeerId;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Kind {
    Friend,
    Out,
    In,
    Blocked,
}

impl Kind {
    fn as_str(self) -> &'static str {
        match self {
            Kind::Friend => "friend",
            Kind::Out => "out",
            Kind::In => "in",
            Kind::Blocked => "blocked",
        }
    }

    fn parse(text: &str) -> Option<Kind> {
        Some(match text {
            "friend" => Kind::Friend,
            "out" => Kind::Out,
            "in" => Kind::In,
            "blocked" => Kind::Blocked,
            _ => return None,
        })
    }
}

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Entry {
    pub kind: Kind,
    /// The peer's display name as last heard (request, reply or presence).
    pub name: String,
    /// The request's note: ours for `Out` (resent on reconnect), theirs for `In`.
    pub note: String,
}

pub struct FriendStore {
    path: PathBuf,
    entries: HashMap<PeerId, Entry>,
}

/// Names and notes share a tab-separated line: no tabs or line breaks.
fn one_line(text: &str) -> String {
    text.chars().map(|c| if matches!(c, '\t' | '\n' | '\r') { ' ' } else { c }).collect()
}

impl FriendStore {
    pub fn load(data_dir: &Path) -> Self {
        let path = data_dir.join("friends.txt");
        let mut entries = HashMap::new();
        if let Ok(text) = std::fs::read_to_string(&path) {
            for line in text.lines() {
                if line.trim().is_empty() || line.starts_with('#') {
                    continue;
                }
                let (peer, rest) = line.split_once(' ').unwrap_or((line.trim(), ""));
                let peer = match PeerId::from_str(peer.trim()) {
                    Ok(peer) => peer,
                    Err(err) => {
                        tracing::warn!(line, %err, "bad friends.txt entry skipped");
                        continue;
                    }
                };
                let (kind, rest) = rest.split_once(' ').unwrap_or((rest, ""));
                let kind = if kind.is_empty() { Some(Kind::Friend) } else { Kind::parse(kind) };
                let Some(kind) = kind else {
                    tracing::warn!(line, "bad friends.txt kind skipped");
                    continue;
                };
                let (name, note) = rest.split_once('\t').unwrap_or((rest, ""));
                entries.insert(peer, Entry { kind, name: name.to_string(), note: note.to_string() });
            }
        }
        Self { path, entries }
    }

    fn save(&self) {
        let mut lines: Vec<String> = self
            .entries
            .iter()
            .map(|(peer, e)| format!("{} {} {}\t{}\n", peer.to_base58(), e.kind.as_str(), e.name, e.note))
            .collect();
        lines.sort();
        let mut text = String::from("# rexnet friend list: peer id, kind, name, tab, note\n");
        text.extend(lines);
        if let Some(parent) = self.path.parent() {
            let _ = std::fs::create_dir_all(parent);
        }
        if let Err(err) = std::fs::write(&self.path, text) {
            tracing::warn!(%err, "failed to persist friend list");
        }
    }

    pub fn get(&self, peer: &PeerId) -> Option<&Entry> {
        self.entries.get(peer)
    }

    pub fn kind(&self, peer: &PeerId) -> Option<Kind> {
        self.entries.get(peer).map(|e| e.kind)
    }

    pub fn is_friend(&self, peer: &PeerId) -> bool {
        self.kind(peer) == Some(Kind::Friend)
    }

    pub fn name(&self, peer: &PeerId) -> String {
        self.entries.get(peer).map(|e| e.name.clone()).unwrap_or_default()
    }

    pub fn friends(&self) -> impl Iterator<Item = &PeerId> {
        self.entries.iter().filter(|(_, e)| e.kind == Kind::Friend).map(|(p, _)| p)
    }

    pub fn entries(&self) -> impl Iterator<Item = (&PeerId, &Entry)> {
        self.entries.iter()
    }

    /// Record `peer` as `kind`. `None` keeps the stored name or note.
    pub fn set(&mut self, peer: PeerId, kind: Kind, name: Option<&str>, note: Option<&str>) {
        let old = self.entries.get(&peer);
        let entry = Entry {
            kind,
            name: name.map(one_line).unwrap_or_else(|| old.map(|e| e.name.clone()).unwrap_or_default()),
            note: note.map(one_line).unwrap_or_else(|| old.map(|e| e.note.clone()).unwrap_or_default()),
        };
        if self.entries.get(&peer) != Some(&entry) {
            self.entries.insert(peer, entry);
            self.save();
        }
    }

    /// Keep a known peer's name current; unknown peers are not added.
    pub fn set_name(&mut self, peer: &PeerId, name: &str) {
        if let Some(kind) = self.kind(peer) {
            self.set(*peer, kind, Some(name), None);
        }
    }

    /// Now a friend; the request's note is no longer needed.
    pub fn add(&mut self, peer: PeerId) {
        self.set(peer, Kind::Friend, None, Some(""));
    }

    pub fn remove(&mut self, peer: &PeerId) {
        if self.entries.remove(peer).is_some() {
            self.save();
        }
    }
}


#[cfg(test)]
mod tests {
    use super::*;

    fn temp_dir(name: &str) -> PathBuf {
        let dir = std::env::temp_dir()
            .join(format!("rexnet-friends-{name}-{}-{}", std::process::id(), rand::random::<u64>()));
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    fn peer() -> PeerId {
        libp2p::identity::Keypair::generate_ed25519().public().to_peer_id()
    }

    #[test]
    fn every_kind_round_trips_with_its_name_and_note() {
        let dir = temp_dir("round");
        let peers = [peer(), peer(), peer(), peer()];
        let mut store = FriendStore::load(&dir);
        store.set(peers[0], Kind::Friend, Some("Speed Machine"), None);
        store.set(peers[1], Kind::Out, Some(""), Some("race me\tsometime\nok"));
        store.set(peers[2], Kind::In, Some("Dave"), Some("hi"));
        store.set(peers[3], Kind::Blocked, Some("Spam"), None);
        let store = FriendStore::load(&dir);
        assert_eq!(
            store.get(&peers[0]),
            Some(&Entry { kind: Kind::Friend, name: "Speed Machine".into(), note: String::new() })
        );
        // One line per peer: tabs and line breaks in text become spaces.
        assert_eq!(store.get(&peers[1]).unwrap().note, "race me sometime ok");
        assert_eq!((store.kind(&peers[2]), store.name(&peers[2])), (Some(Kind::In), "Dave".to_string()));
        assert_eq!(store.kind(&peers[3]), Some(Kind::Blocked));
        assert_eq!(store.friends().collect::<Vec<_>>(), vec![&peers[0]]);
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn an_old_one_column_file_loads_as_friends_and_bad_lines_are_skipped() {
        let dir = temp_dir("old");
        let friend = peer();
        let text = format!("# old list\n{}\nnot-a-peer\n{} weird kind\n", friend.to_base58(), peer().to_base58());
        std::fs::write(dir.join("friends.txt"), text).unwrap();
        let store = FriendStore::load(&dir);
        assert!(store.is_friend(&friend));
        assert_eq!(store.entries().count(), 1);
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn a_name_update_keeps_kind_and_note_and_remove_forgets_the_peer() {
        let dir = temp_dir("name");
        let p = peer();
        let mut store = FriendStore::load(&dir);
        store.set(p, Kind::Out, None, Some("hello"));
        store.set_name(&p, "Bob");
        assert_eq!(store.get(&p), Some(&Entry { kind: Kind::Out, name: "Bob".into(), note: "hello".into() }));
        store.set_name(&peer(), "nobody"); // unknown peers are not added
        assert_eq!(store.entries().count(), 1);
        store.add(p);
        assert_eq!(store.get(&p), Some(&Entry { kind: Kind::Friend, name: "Bob".into(), note: String::new() }));
        store.remove(&p);
        assert!(FriendStore::load(&dir).get(&p).is_none());
        std::fs::remove_dir_all(dir).unwrap();
    }
}
