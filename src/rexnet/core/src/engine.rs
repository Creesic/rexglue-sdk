// @file        rexnet/core/src/engine.rs
// @brief       Async engine: the libp2p swarm, the game-plane UDP socket, and the.
//
// @copyright   Copyright (c) 2026 Ryan Fisher <ryanfisher099@gmail.com>
//              All rights reserved.
//
// @license     BSD 3-Clause License
//              See LICENSE file in the project root for full license text.

//! Async engine: the libp2p swarm, the game-plane UDP socket, and the
//! command/event loop.
//!
//! Control plane per design spec §5/§6: QUIC (+TCP fallback) transport,
//! Kademlia peer routing (Amino-compatible, client mode), identify, AutoNAT,
//! circuit-v2 relay client, DCUtR hole punching, and LAN mDNS. The FFI layer
//! enqueues [`Command`]s and drains [`Event`]s; the CLI drives the same
//! engine directly (Rust-level commands carry oneshot reply channels).
//!
//! Game plane per §6/§8.4: a dedicated UDP socket multiplexing all guest
//! ports, established per-peer by a simultaneous-open punch coordinated over
//! the control connection. Every peer gets a virtual IP in 10.77.0.0/16
//! (allocated here, mirrored by the C++ shim via `PeerConnected`).
//!
//! The §7 presence/friend/invite/session protocols, the encrypted game plane
//! (§6), the §5 tunnel fallback for hostile-NAT pairs, the §17 ambient shard,
//! and the §9 mesh-relay register/discovery are all implemented; see the
//! per-item comments below.

use std::collections::{HashMap, HashSet};
use std::net::{IpAddr, SocketAddr};
use std::path::PathBuf;
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::Arc;
use std::time::{Duration, Instant};

use libp2p::futures::StreamExt;
use libp2p::identity::Keypair;
use libp2p::kad::store::MemoryStore;
use libp2p::multiaddr::Protocol;
use libp2p::request_response::{self, OutboundRequestId, ProtocolSupport};
use libp2p::swarm::dial_opts::{DialOpts, PeerCondition};
use libp2p::swarm::{ConnectionId, NetworkBehaviour, SwarmEvent};
use libp2p::{
    autonat, connection_limits, dcutr, gossipsub, identify, kad, mdns, relay, upnp, Multiaddr,
    PeerId, StreamProtocol, Swarm,
};
use serde::{Deserialize, Serialize};
use tokio::net::UdpSocket;
use tokio::sync::{mpsc, oneshot, Notify};

use crate::friends::{FriendStore, Kind};
use crate::shard::{self, Candidate, Migration, Placement, ShardDesc, DEFAULT_SHARD_CAP};
use crate::subnet;
use crate::tcp::{StreamHeader, StreamId, StreamInfo, StreamTable, STREAM_HEADER_LEN, STREAM_SCHEMA};
use crate::crypto::{self, LocalKeyAgreement};
use crate::game_plane::{Emitter, GamePlane};
use crate::tunnel;

pub type BoxError = Box<dyn std::error::Error + Send + Sync>;

/// Base of the virtual-IP network handed to peers: 10.77.0.0/16 (§11).
pub const VIP_NETWORK_BASE: u32 = (10 << 24) | (77 << 16);

/// Dotted-quad rendering for logs; virtual IPs are held in host order.
fn format_vip(vip: u32) -> String {
    format!(
        "{}.{}.{}.{}",
        (vip >> 24) & 0xFF,
        (vip >> 16) & 0xFF,
        (vip >> 8) & 0xFF,
        vip & 0xFF
    )
}

/// Reduce a listen/external multiaddr to the bare `host:port` a player pastes
/// into the direct-connect field. IPv6 is bracketed so the port stays
/// unambiguous. Returns `None` for addresses without an ip+port pair (e.g. a
/// relay circuit address), which are not directly dialable.
fn dialable_endpoint(addr: &Multiaddr) -> Option<String> {
    let mut host: Option<String> = None;
    let mut port: Option<u16> = None;
    for proto in addr.iter() {
        match proto {
            Protocol::Ip4(ip) => host = Some(ip.to_string()),
            Protocol::Ip6(ip) => host = Some(format!("[{ip}]")),
            Protocol::Udp(p) | Protocol::Tcp(p) => port = Some(p),
            _ => {}
        }
    }
    Some(format!("{}:{}", host?, port?))
}

/// Conservative "reachable from the public internet" test for a multiaddr:
/// reject loopback, RFC1918 private, link-local, ULA and unspecified ranges.
/// Used so an auto relay reservation is only held on a peer other NAT'd peers
/// could actually reach us through.
fn multiaddr_is_global(addr: &Multiaddr) -> bool {
    for proto in addr.iter() {
        match proto {
            Protocol::Ip4(ip) => {
                return !(ip.is_loopback()
                    || ip.is_private()
                    || ip.is_link_local()
                    || ip.is_unspecified()
                    || ip.is_broadcast()
                    || ip.is_documentation());
            }
            Protocol::Ip6(ip) => {
                // Stable Rust has no is_unique_local/is_global for v6, so test
                // the fc00::/7 ULA and fe80::/10 link-local prefixes by hand.
                let seg = ip.segments();
                let is_ula = (seg[0] & 0xfe00) == 0xfc00;
                let is_link_local = (seg[0] & 0xffc0) == 0xfe80;
                return !(ip.is_loopback() || ip.is_unspecified() || is_ula || is_link_local);
            }
            _ => {}
        }
    }
    false
}

/// Game-plane frame types (first byte on the game socket).
/// Smallest RTT change worth telling the shim about; below this it is noise.
const RTT_REPORT_DELTA_MS: u32 = 5;

/// `[type][nonce 16][echo 8]`.
const PROBE_FRAME_LEN: usize = 25;

/// Microseconds since the process started. Only ever compared with itself.
fn now_micros() -> u64 {
    static START: std::sync::OnceLock<Instant> = std::sync::OnceLock::new();
    START.get_or_init(Instant::now).elapsed().as_micros() as u64
}

pub const FRAME_DATA: u8 = 0x01;
const FRAME_PROBE: u8 = 0x02;
const FRAME_PROBE_ACK: u8 = 0x03;

/// Shard rescan cadence and migration cooldown (§17.3.3, §17.6 open item:
/// untuned). The sweep is cheap -- one provider lookup -- but a migration is
/// a resubscribe, so moves are rate-limited far more aggressively than scans.
/// How often a relay with no live reservation is re-dialled. Slow on purpose:
/// relays are an accelerant, and a node with none still works.
const RELAY_RETRY_INTERVAL: Duration = Duration::from_secs(60);

/// Cap on automatic circuit reservations held on mesh peers (§9 torrent model).
/// A couple is enough redundancy for DCUtR to find a rendezvous path; more
/// would turn a large swarm into an N-squared reservation storm.
const MAX_AUTO_RELAYS: usize = 2;
/// A relay that refused or failed our reservation is not picked again for this long.
const AUTO_RELAY_BACKOFF: Duration = Duration::from_secs(600);
/// How long we hold out for a RexNet relay before taking any other.
const AUTO_RELAY_REXNET_WAIT: Duration = Duration::from_secs(20);

/// Whether an identified peer offering the hop on a global address may take
/// a free auto-relay slot:
/// - never a bootstrap peer: Amino's advertise the hop, never accept, and held
///   both slots for 92-138 s;
/// - never one still backing off after refusing us;
/// - a RexNet relay (it speaks the shard protocol, so its relay server takes
///   our reservation) at once;
/// - any other only while we hold no RexNet relay and have had none for
///   AUTO_RELAY_REXNET_WAIT, counted from start or from losing the last one.
fn auto_relay_eligible(
    bootstrap: bool,
    backoff_until: Option<Instant>,
    rexnet: bool,
    holding_rexnet: bool,
    no_rexnet_since: Instant,
    now: Instant,
) -> bool {
    !bootstrap
        && backoff_until.is_none_or(|until| now >= until)
        && (rexnet || (!holding_rexnet && now.saturating_duration_since(no_rexnet_since) >= AUTO_RELAY_REXNET_WAIT))
}

/// The address to reserve an auto relay through: a global one that is not a
/// circuit. A relay that holds a reservation itself advertises that circuit,
/// a circuit inside a circuit cannot be listened on, and identify lists
/// addresses in no set order, so taking the first global one failed at random.
fn reservable_addr(listen_addrs: &[Multiaddr]) -> Option<&Multiaddr> {
    listen_addrs.iter().find(|a| multiaddr_is_global(a) && !a.iter().any(|p| matches!(p, Protocol::P2pCircuit)))
}

/// Identified rexnet nodes, shared with the relay server's limiters.
type RelayPeers = Arc<std::sync::Mutex<HashSet<PeerId>>>;

fn game_relay_config(peers: RelayPeers) -> relay::Config {
    // Keep upstream reservation, concurrency and rate limits. A game circuit
    // needs more than the rendezvous default of two minutes / 128 KiB, but
    // only for other rexnet nodes: the public DHT peers we also connect to
    // must not get two-hour, 256 MiB circuits through us.
    let mut config = relay::Config {
        max_circuit_duration: Duration::from_secs(2 * 60 * 60),
        max_circuit_bytes: 256 * 1024 * 1024,
        ..relay::Config::default()
    };
    let only_rexnet = |peers: RelayPeers| Box::new(move |peer: PeerId, _: &libp2p::Multiaddr, _: Instant|
        peers.lock().unwrap_or_else(|error| error.into_inner()).contains(&peer));
    config.reservation_rate_limiters.push(only_rexnet(peers.clone()));
    config.circuit_src_rate_limiters.push(only_rexnet(peers));
    config
}

// Keep known small-shard peers warm and retry failed dials without waiting
// for a 30-second presence beat. One outstanding keepalive per peer.
const LINK_MAINTENANCE_INTERVAL: Duration = Duration::from_secs(5);
/// Failed dials back off from LINK_MAINTENANCE_INTERVAL up to this, so a
/// member that vanished without unsubscribing is not dialled every 5 s.
const SHARD_RETRY_MAX: Duration = Duration::from_secs(60);
// Recover within the title's existing liveness budget; never extend a failed
// attempt by reconnecting only its control plane, or by failing again.
const PEER_RECOVERY_GRACE: Duration = Duration::from_secs(5);
const PEER_RECOVERY_TICK: Duration = Duration::from_millis(250);
const PEER_RECOVERY_RETRY: Duration = Duration::from_secs(1);
const ROUTE_CACHE_TTL: Duration = Duration::from_secs(600);
const ROUTE_CACHE_PEERS: usize = MAX_ESTABLISHED_CONNECTIONS as usize;
const ROUTE_CACHE_ADDRS: usize = 8;

struct PeerRoutes {
    updated: Instant,
    addresses: Vec<Multiaddr>,
    avoid_quic_until: Option<Instant>,
}

struct PeerRecovery {
    started: Instant,
    next_dial: Instant,
    lookup: Option<kad::QueryId>,
}

fn direct_quic(addr: &Multiaddr) -> bool {
    addr.iter().any(|p| matches!(p, Protocol::QuicV1))
        && !addr.iter().any(|p| matches!(p, Protocol::P2pCircuit))
}

fn route_priority(addr: &Multiaddr) -> u8 {
    if addr.iter().any(|p| matches!(p, Protocol::P2pCircuit)) { 1 }
    else if direct_quic(addr) { 2 } else { 0 }
}

/// Only cache addresses our transports understand. An accepted TCP source
/// port is not a listening address; incoming QUIC uses its listening endpoint.
fn reconnect_address(endpoint: &libp2p::core::ConnectedPoint) -> Option<Multiaddr> {
    (endpoint.is_dialer() || direct_quic(endpoint.get_remote_address()))
        .then(|| endpoint.get_remote_address().clone())
}

fn remember_routes(cache: &mut HashMap<PeerId, PeerRoutes>, peer: PeerId,
    addresses: impl IntoIterator<Item = Multiaddr>, now: Instant) {
    cache.retain(|_, routes| now.saturating_duration_since(routes.updated) < ROUTE_CACHE_TTL);
    for addr in addresses {
        if addr.len() > 512 || !addr.iter().any(|p| matches!(p, Protocol::Tcp(n) if n != 0)
            || matches!(p, Protocol::QuicV1))
            || addr.iter().any(|p| matches!(p, Protocol::Ws(_) | Protocol::Wss(_) | Protocol::WebTransport)
                || matches!(p, Protocol::Ip4(ip) if ip.is_unspecified())
                || matches!(p, Protocol::Ip6(ip) if ip.is_unspecified())
                || matches!(p, Protocol::Tcp(0) | Protocol::Udp(0)))
            || matches!(addr.iter().last(), Some(Protocol::P2p(id)) if id != peer)
        { continue; }
        if !cache.contains_key(&peer) && cache.len() >= ROUTE_CACHE_PEERS {
            if let Some(oldest) = cache.iter().min_by_key(|(_, r)| r.updated).map(|(p, _)| *p) {
                cache.remove(&oldest);
            }
        }
        let routes = cache.entry(peer).or_insert_with(|| PeerRoutes {
            updated: now, addresses: Vec::new(), avoid_quic_until: None,
        });
        routes.updated = now;
        routes.addresses.retain(|old| old != &addr);
        routes.addresses.insert(0, addr);
        routes.addresses.sort_by_key(route_priority);
        // Numerous local NIC addresses must not crowd out the relay or the
        // successful QUIC endpoint. Reserve one entry for each available path.
        let mut distinct: Vec<_> = (0..3).filter_map(|priority|
            routes.addresses.iter().find(|a| route_priority(a) == priority).cloned()).collect();
        for addr in routes.addresses.drain(..) {
            if distinct.len() < ROUTE_CACHE_ADDRS && !distinct.contains(&addr) { distinct.push(addr); }
        }
        routes.addresses = distinct;
        routes.addresses.sort_by_key(route_priority);
    }
}

fn reconnect_routes(routes: Option<&PeerRoutes>, now: Instant) -> Vec<Multiaddr> {
    let Some(routes) = routes.filter(|r| now.saturating_duration_since(r.updated) < ROUTE_CACHE_TTL)
        else { return Vec::new() };
    let alternatives = routes.addresses.iter().any(|a| !direct_quic(a));
    let avoid_quic = alternatives && routes.avoid_quic_until.is_some_and(|at| now < at);
    let mut addresses: Vec<_> = routes.addresses.iter()
        .filter(|a| !avoid_quic || !direct_quic(a)).cloned().collect();
    // TCP to the peer, then a different route through a relay, then QUIC.
    addresses.sort_by_key(route_priority);
    addresses
}
/// Large shards only resolve addresses. Once per member per ten minutes;
/// every beat would be a DHT walk per member every 30 s.
const SHARD_RESOLVE_INTERVAL: Duration = Duration::from_secs(600);
/// A member that neither beats nor holds a connection for three beats has left.
const SHARD_MEMBER_TIMEOUT: Duration = Duration::from_secs(3 * PRESENCE_HEARTBEAT.as_secs());

/// Whether to retry `peer` now. Each retry doubles the wait, from `first` up to `max`.
fn shard_retry_due(retries: &mut HashMap<PeerId, (Instant, Duration)>, peer: PeerId, now: Instant,
    first: Duration, max: Duration) -> bool {
    let delay = match retries.get(&peer) {
        Some(&(next, _)) if now < next => return false,
        Some(&(_, delay)) => (delay * 2).min(max),
        None => first,
    };
    retries.insert(peer, (now + delay, delay));
    true
}

/// Friends and requests we are not connected to are looked up in the DHT and
/// dialled, at once and then from 30 s, doubling to 10 min while they stay away.
const FRIEND_LOOKUP_FIRST: Duration = Duration::from_secs(30);
const FRIEND_LOOKUP_MAX: Duration = Duration::from_secs(600);

/// Our own reachability record is published at each heartbeat until a publish
/// is stored by at least one DHT node, then refreshed every ten minutes so
/// new addresses and lost stores recover (kad's own republish is 12 h).
const OWN_KEY_REPUBLISH: Duration = Duration::from_secs(600);

fn own_key_due(last_stored: Option<Instant>, now: Instant) -> bool {
    last_stored.map_or(true, |at| now.saturating_duration_since(at) >= OWN_KEY_REPUBLISH)
}

/// Stored peers worth finding: friends, requests either way, and peers with
/// messages waiting. Not blocked, not already connected.
fn lookup_targets(store: &FriendStore, outbox: &[PeerId], connected: &HashSet<PeerId>) -> Vec<PeerId> {
    let mut targets: HashSet<PeerId> = store
        .entries()
        .filter(|(peer, entry)| entry.kind != Kind::Blocked && !connected.contains(*peer))
        .map(|(peer, _)| *peer)
        .collect();
    targets.extend(
        outbox.iter().filter(|p| store.kind(p) != Some(Kind::Blocked) && !connected.contains(*p)).copied(),
    );
    targets.into_iter().collect()
}

/// UPnP lease on the game socket, renewed at half-life as libp2p-upnp does.
const GAME_PORT_LEASE: Duration = Duration::from_secs(3600);

/// Map the game socket on a UPnP router. libp2p-upnp maps only the swarm's
/// listeners, so a NAT'd peer's probes to our public game port were dropped
/// and the pair fell back to a relayed tunnel even though we were reachable.
async fn map_game_port(port: u16) {
    use igd_next::{aio::tokio::search_gateway, PortMappingProtocol, SearchOptions};
    let gateway = match search_gateway(SearchOptions::default()).await {
        Ok(gateway) => gateway,
        Err(error) => {
            tracing::info!(?error, "upnp: game port not mapped (no gateway)");
            return;
        }
    };
    // The address our route to the gateway leaves from is the one it can forward to.
    let local = std::net::UdpSocket::bind(("0.0.0.0", 0))
        .and_then(|socket| socket.connect(gateway.addr).and_then(|()| socket.local_addr()));
    let local_ip = match local {
        Ok(addr) => addr.ip(),
        Err(error) => {
            tracing::warn!(?error, "upnp: no route to the gateway; game port not mapped");
            return;
        }
    };
    loop {
        let internal = SocketAddr::new(local_ip, port);
        let lease = GAME_PORT_LEASE.as_secs() as u32;
        match gateway.add_port(PortMappingProtocol::UDP, port, internal, lease, "rexnet game").await {
            Ok(()) => tracing::info!(port, %internal, "upnp: game port mapped"),
            Err(error) => {
                tracing::warn!(port, ?error, "upnp: game port mapping failed");
                return;
            }
        }
        tokio::time::sleep(GAME_PORT_LEASE / 2).await;
    }
}

/// One control connection to a peer. A relayed one carries the point at which
/// it should be replaced: half its circuit's time and data budget, because the
/// tunnel is not the only traffic on it and its bytes are all we count.
struct Link {
    id: ConnectionId,
    relayed: bool,
    quic: bool,
    renew_at: Option<Instant>,
    renew_bytes: Option<u64>,
    /// The address we dialled, or None for a link we accepted: its remote port
    /// is the peer's outgoing one, not one it listens on.
    remote: Option<Multiaddr>,
    /// The peer dialled us.
    inbound: bool,
}

/// The relay a dialled circuit runs through: the /p2p/ before /p2p-circuit.
fn circuit_relay(addr: &Multiaddr) -> Option<PeerId> {
    let mut relay = None;
    for proto in addr.iter() {
        match proto {
            Protocol::P2p(id) => relay = Some(id),
            Protocol::P2pCircuit => return relay,
            _ => {}
        }
    }
    None
}

/// Only relayed links age out; the newest one is the one still carrying us.
fn renewal_due(links: &[Link], now: Instant, tunnel_bytes: u64) -> bool {
    links.iter().all(|link| link.relayed)
        && links.last().is_some_and(|newest| {
            newest.renew_at.is_some_and(|at| now >= at)
                || newest.renew_bytes.is_some_and(|bytes| tunnel_bytes >= bytes)
        })
}

/// Relayed links a newer or direct link makes redundant. Streams pick a
/// connection at random, so a spare relayed link would keep taking the tunnel
/// until its circuit expired under it.
fn superseded(links: &[Link]) -> Vec<ConnectionId> {
    let keep_newest = !links.iter().any(|link| !link.relayed);
    let last = links.len().saturating_sub(1);
    links
        .iter()
        .enumerate()
        .filter(|(i, link)| link.relayed && !(keep_newest && *i == last))
        .map(|(_, link)| link.id)
        .collect()
}

fn quic_links_replaced(links: &[Link], replacement: &Multiaddr, game_active: bool) -> Vec<ConnectionId> {
    if !game_active || direct_quic(replacement) { return Vec::new(); }
    links.iter().filter(|l| l.quic && !l.relayed).map(|l| l.id).collect()
}

fn departed_members(heard: &HashMap<PeerId, Instant>, connected: &HashSet<PeerId>, now: Instant) -> Vec<PeerId> {
    heard.iter()
        .filter(|(peer, at)| !connected.contains(peer) && now.duration_since(**at) > SHARD_MEMBER_TIMEOUT)
        .map(|(peer, _)| *peer)
        .collect()
}

/// A dial already in flight will report its own outcome; only a peer with no
/// known address needs a DHT walk.
fn shard_dial_needs_dht(error: &libp2p::swarm::DialError) -> bool {
    matches!(error, libp2p::swarm::DialError::NoAddresses)
}

const SHARD_RESCAN_INTERVAL: Duration = Duration::from_secs(20);
/// How often a beacon (`Command::Beacon`) is announced again. kad's own
/// republish is every 12 h, but an announcement carries the addresses we had
/// when it was sent, and a relay circuit changes whenever its reservation does.
const BEACON_REPUBLISH: Duration = Duration::from_secs(20 * 60);
/// How long to keep looking after a fruitless sweep before founding a shard.
///
/// The sweep can finish before a peer that is already up has connected and
/// identified -- on LAN that gap was measured at ~6 s -- and a node that founds
/// a shard inside that window abandons it moments later when the peer's
/// descriptor finally arrives. Convergence handles the collision correctly, but
/// every hop re-rolls the shard id and therefore the subnet, re-addressing the
/// whole membership for nothing.
///
/// Costs a node that really is alone a short wait before ambient presence
/// starts; it blocks nothing else, and friend invites do not go through here.
const SHARD_CREATE_GRACE: Duration = Duration::from_secs(10);
/// Below this many members, migrate without the jitter roll. Jitter exists to
/// stop a large shard stampeding into another one; with a handful of members
/// there is no stampede to prevent, and the roll just leaves two players
/// sitting in separate shards for minutes -- which defeats the whole point of
/// the tier.
const SHARD_JITTER_MIN_MEMBERS: usize = 5;
const SHARD_MIGRATION_COOLDOWN: Duration = Duration::from_secs(300);
/// Probability a node acts on a migration decision in any given rescan.
/// Every member of a shard reaches the same conclusion from the same DHT
/// state at roughly the same moment, so without this they would all
/// resubscribe in lockstep -- a self-inflicted thundering herd on the shard
/// they are moving to. Direction is deliberately unconditional (§17.3.3),
/// which leaves spreading the *timing* as the only defence. Deferring
/// re-rolls next rescan, so a population drains over several intervals
/// (~half each) rather than in one instant.
const SHARD_MIGRATION_CHANCE: f64 = 0.5;
/// While a shard is at most this many members, hold a control connection to
/// every one of them. Gossipsub only meshes with ~6 peers regardless of topic
/// size, so membership alone leaves most of a shard unreachable: no address,
/// no connection, and an invite to them cold-starts with a DHT lookup that
/// often fails outright. Below this size the connections are cheap and every
/// member is instantly invitable -- which is the point of the Connected tier.
/// Above it we fall back to resolving addresses only, so a 255-member shard
/// does not mean 255 connections.
const SHARD_WARM_CONNECT_LIMIT: usize = 24;
/// How long an invite may wait for its peer to become reachable. An invite
/// that finally lands minutes after the user asked is worse than none: they
/// have moved on, and the session it names may be gone. Matches the 30 minute
/// expiry carried in the invite itself.
const INVITE_DEFER_MAX: Duration = Duration::from_secs(1800);
/// Datagrams held per peer while its game-plane punch completes, and how long
/// they stay worth sending.
///
/// A title resolves a peer's address and immediately sends to it -- Fable 2
/// fires its join handshake in the same millisecond as XNetXnAddrToInAddr --
/// but the punch needs a round trip. Dropping what arrives in that window
/// loses the *first* message of a conversation, which is invariably the one
/// that starts it, and titles rarely retry a join handshake. The cap bounds a
/// peer that never punches; the age bound stops a stale handshake arriving
/// long after the title gave up, which would confuse it more than silence.
const PUNCH_QUEUE_MAX: usize = 32;
/// Must exceed PUNCH_TIMEOUT. Held datagrams are flushed when the punch
/// resolves -- either by succeeding, or by timing out into the tunnel at
/// PUNCH_TIMEOUT. With this shorter than that timeout, a slow punch meant
/// every queued frame aged out microseconds before the tunnel could carry it,
/// so the fallback delivered nothing and the join handshake was lost anyway.
const PUNCH_QUEUE_MAX_AGE: Duration = Duration::from_secs(12);
/// How long to wait for a punch before falling back to the tunnel (§14).
/// Slightly longer than the probe run (PROBE_ROUNDS * PROBE_INTERVAL) so a
/// late-but-successful punch is still preferred over the degraded path.
const PUNCH_TIMEOUT: Duration = Duration::from_secs(6);
// Invariant: PUNCH_QUEUE_MAX_AGE > PUNCH_TIMEOUT, or the tunnel fallback has
// nothing left to send by the time it engages.
const _: () = assert!(
    PUNCH_QUEUE_MAX_AGE.as_secs() > PUNCH_TIMEOUT.as_secs(),
    "queued datagrams must outlive the punch timeout"
);
/// How often to check for punches that have run out of time.
const PUNCH_WATCHDOG_INTERVAL: Duration = Duration::from_secs(1);
/// How often to print the one-line shard summary. Frequent enough to watch a
/// shard form, rare enough not to become noise itself.
const SHARD_STATUS_INTERVAL: Duration = Duration::from_secs(30);

/// A datagram waiting for its peer's punch to resolve.
struct PendingDatagram {
    queued_at: Instant,
    max_age: Duration,
    src_port: u16,
    dst_port: u16,
    data: Vec<u8>,
}

/// How long and how often both sides blast punch probes (§8.4: up to 5 s).
const PROBE_INTERVAL: Duration = Duration::from_millis(100);
const PROBE_ROUNDS: u32 = 50;

/// Commands enqueued by the FFI layer (game thread side, non-blocking).
/// The oneshot-carrying variants are Rust-level (CLI/tests); the FFI surface
/// never constructs them.
// Several payloads are carried but unread until the §7 protocols land.
#[allow(dead_code)]
#[derive(Debug)]
pub enum Command {
    SetPresence { title_id: u32, state: u8, rich: Vec<u8> },
    /// Change the self-asserted display name at runtime (overlay rename);
    /// re-pushes presence so friends see it immediately.
    SetDisplayName { name: String },
    ConnectPeer { peer: PeerId },
    ConnectManual { multiaddr: String },
    /// Invite `peer` to the current local session (§8.3).
    SendInvite { peer: PeerId, note: String },
    /// Reply to a received invite. Accepting also fetches the host's
    /// session descriptor so the shim can build join material.
    InviteReply { peer: PeerId, accept: bool },
    /// Withdraw our invite to `peer` (cancelled, or our session ended).
    RevokeInvite { peer: PeerId },
    /// Ask one peer for its session descriptor; the answer is a SessionFound.
    SessionQuery { peer: PeerId },
    /// Queue a title message for `peer`; it is delivered whenever they are
    /// connected and kept until acknowledged.
    MessageSend { peer: PeerId, id: [u8; 16], blob: Vec<u8> },
    /// We own `team`: save this version, push it to connected members (and
    /// to anyone just removed), and keep `name` for the DHT.
    ClubPublish { team: u64, version: u64, record: Vec<u8>, members: Vec<PeerId>, name: Option<String> },
    /// Joined `team`, not yet confirmed: track it with a provisional record.
    ClubTrack { team: u64, owner: PeerId, record: Vec<u8> },
    /// Left `team`.
    ClubForget { team: u64 },
    /// We deleted `team`.
    ClubDelete { team: u64 },
    /// Send a club request to its owner, kept until acknowledged.
    ClubRequest { owner: PeerId, id: [u8; 16], blob: Vec<u8> },
    /// Hand `team` to `new_owner`: we become a member of it.
    ClubHandover { team: u64, new_owner: PeerId, id: [u8; 16], version: u64, record: Vec<u8>, members: Vec<PeerId> },
    /// Is `name` advertised by another club's owner? Answered by ClubNameResult.
    ClubNameCheck { id: u32, name: String },
    FriendRequest { peer: PeerId, note: String },
    FriendAccept { peer: PeerId },
    FriendRemove { peer: PeerId },
    /// Decline a received request; `block` also drops their later requests.
    FriendDecline { peer: PeerId, block: bool },
    /// Register the local session (id allocated by the shim). Public
    /// sessions additionally publish a DHT provider record (§10).
    SessionCreate { id: [u8; 16], slots_total: u8, slots_open: u8, is_public: bool },
    SessionDelete,
    /// Replace our session's slot counts and data; queriers see it next.
    SessionUpdate { slots_total: u8, slots_open: u8, data: Vec<u8> },
    /// Echo a peer and report the round trip as Event::EchoResult.
    EchoProbe { peer: PeerId },
    /// Find public sessions for our title via DHT provider records, then
    /// query each advertiser; results arrive as [`Event::SessionFound`].
    SessionSearch,
    /// Opt in to the ambient title shard (§17.3, §17.5). Idempotent; a cap of
    /// 0 means the spec default.
    ShardEnable { cap: u16 },
    /// Opt out: leave any shard and stop publishing/scanning.
    ShardDisable,
    /// Which shard we are in, if any. Rust-level (CLI/tests/diagnostics).
    ShardStatus { reply: oneshot::Sender<Option<[u8; 16]>> },
    /// Guest TCP connect to a peer's virtual IP (§18).
    StreamConnect { virtual_ip: u32, src_port: u16, dst_port: u16 },
    /// Guest wrote to a connected stream socket.
    StreamSend { stream_id: StreamId, data: Vec<u8> },
    /// Guest closed a stream socket.
    StreamClose { stream_id: StreamId },
    SendDatagram { virtual_ip: u32, src_port: u16, dst_port: u16, data: Vec<u8>, reliable: bool },
    /// `reply` is None for fire-and-forget (FFI); results also surface as
    /// [`Event::PunchResult`].
    Punch { peer: PeerId, reply: Option<oneshot::Sender<Result<SocketAddr, String>>> },
    Echo { peer: PeerId, reply: oneshot::Sender<Result<Duration, String>> },
    FindPeer { peer: PeerId, reply: oneshot::Sender<Result<Vec<Multiaddr>, String>> },
    LocalAddrs { reply: oneshot::Sender<Vec<Multiaddr>> },
    /// Keep a signed notice as if a peer had sent it; a newer broadcast is
    /// then passed on. Rust-level (the rexnet-notice panel).
    PublishNotice { signed: crate::notice::Signed },
    /// Deliver a signed targeted notice to `peer` directly, looking it up if
    /// needed; kept until acknowledged. Rust-level (panel).
    SendNoticeTo { peer: PeerId, signed: crate::notice::Signed },
    /// Dial everyone providing `title_id`'s shard key, so connection hand-off
    /// carries the broadcast notice into each of its shards. Rust-level (panel).
    SeedTitle { title_id: u32 },
    /// Ask notice peers, everyone providing `title_id`'s shard key, and each
    /// RexNet peer that identifies from now on, who they are; answers arrive as
    /// [`Event::RosterEntry`]. A scan (`refresh` false) skips runners already
    /// reported and still connected; a refresh asks every notice peer again.
    /// Rust-level (panel).
    Roster { title_id: u32, ask: crate::notice::Signed, refresh: bool },
    /// Provide `title_id`'s shard key without joining a shard, so every
    /// runner's shard discovery (every 20 s) dials us and its ShardQuery gets
    /// None. A title already beaconing is skipped. Panel-only.
    Beacon { title_id: u32 },
    /// Dial an address without wanting its peer: unlike ConnectManual, a
    /// failure is logged at debug and never reported. Rust-level (the panel's
    /// remembered players).
    Dial { multiaddr: String },

    #[cfg(test)]
    DropControlForTest { peer: PeerId, reply: oneshot::Sender<()> },
    #[cfg(test)]
    ControlPathsForTest { peer: PeerId, reply: oneshot::Sender<Vec<bool>> },
    #[cfg(test)]
    GameSocketForTest { reply: oneshot::Sender<Arc<UdpSocket>> },
    #[cfg(test)]
    FriendProviderForTest { peer: PeerId, reply: oneshot::Sender<(bool, bool)> },
    Shutdown,
}

/// Events surfaced to the FFI layer, drained once per frame by the shim.
// Some variants unconstructed until the §7 protocols land.
#[allow(dead_code)]
#[derive(Debug)]
pub enum Event {
    /// `virtual_ip` is the 10.77.0.0/16 address allocated for this peer.
    PeerConnected { peer: PeerId, virtual_ip: u32 },
    PeerDisconnected { peer: PeerId },
    PresenceUpdated { peer: PeerId, title_id: u32, state: u8, display_name: String, rich: Vec<u8> },
    FriendRequest { peer: PeerId, display_name: String, note: String },
    /// Also emitted once per stored friend at engine start, so the shim's
    /// mirror converges without a separate list query. `display_name` is the
    /// name last heard.
    FriendAccepted { peer: PeerId, display_name: String },
    /// Our request reached `peer` (now, or replayed at startup while pending).
    FriendRequestSent { peer: PeerId, display_name: String },
    /// Our request could not be delivered; it is dropped.
    FriendRequestFailed { peer: PeerId },
    FriendRemoved { peer: PeerId },
    InviteReceived { peer: PeerId, title_id: u32, session_id: [u8; 16], note: String },
    InviteRevoked { peer: PeerId, session_id: [u8; 16] },
    /// A title message arrived (not a resend, not from a blocked peer).
    MessageReceived { peer: PeerId, blob: Vec<u8> },
    /// A club we own or belong to: at startup, and whenever its record changes.
    ClubUpdated { team: u64, owner: PeerId, version: u64, owned: bool, record: Vec<u8>, members: Vec<PeerId> },
    /// A club we belonged to is gone: deleted, or we are no longer a member.
    ClubGone { team: u64, deleted: bool },
    /// A member's change reached us, the club's owner.
    ClubRequest { peer: PeerId, blob: Vec<u8> },
    /// The answer to ClubNameCheck `id`.
    ClubNameResult { id: u32, taken: bool },
    /// reply: 1 = accepted, 0 = declined.
    InviteReplied { peer: PeerId, reply: u8 },
    PunchResult { peer: PeerId, ok: bool },
    SessionFound {
        host: PeerId,
        session_id: [u8; 16],
        slots_total: u8,
        slots_open: u8,
        requires_invite: bool,
        data: Vec<u8>,
    },
    /// Answer to Command::EchoProbe; None when the echo failed.
    EchoResult { peer: PeerId, rtt_ms: Option<u32> },
    /// A peer asked for our session descriptor (it is searching). `served`
    /// is false when we host nothing it may see. Diagnostic only.
    SessionQueried { peer: PeerId, served: bool },
    /// A guest TCP connection came up: either one we opened (`outbound`) or
    /// one accepted on a listening port.
    StreamOpened {
        stream_id: StreamId,
        virtual_ip: u32,
        local_port: u16,
        remote_port: u16,
        outbound: bool,
    },
    /// Bytes arrived on a guest TCP connection.
    StreamData { stream_id: StreamId, data: Vec<u8> },
    /// A guest TCP connection ended (either side, or the peer vanished).
    StreamClosed { stream_id: StreamId },
    /// An outbound guest TCP connect failed.
    StreamConnectFailed { virtual_ip: u32, dst_port: u16, message: String },
    /// Measured round trip to a peer, milliseconds. Emitted only on a
    /// material change; the shim caches it for QoS.
    PeerRtt { virtual_ip: u32, rtt_ms: u32 },
    /// Our own address on the virtual network changed — we joined, left or
    /// migrated shards (§17.3.5). The shim reports this as the local XNADDR.
    LocalAddress { virtual_ip: u32 },
    /// A directly dialable public endpoint for us was confirmed (UPnP mapping
    /// or AutoNAT). `addr` is a bare `host:port` the player can hand to a peer
    /// for a manual direct connect. Emitted on each newly confirmed endpoint.
    ExternalAddress { addr: String },
    /// A peer started or stopped carrying our traffic as a circuit relay
    /// (§17.2). Drives the `relay` label: a non-friend relay is the one
    /// non-friend the UI names at all, and it is named by role, not identity.
    RelayStatus { peer: PeerId, is_relay: bool },
    /// Ambient shard presence (§17.3.4). `display_name` is deliberately not
    /// carried: the shim renders a pseudonym unless the peer is a friend.
    ShardPresence {
        peer: PeerId,
        state: u8,
        rich: Vec<u8>,
        session_id: Option<[u8; 16]>,
    },
    Datagram { virtual_ip: u32, src_port: u16, dst_port: u16, data: Vec<u8> },
    /// Punch failed; game traffic is tunneled over the control connection.
    Degraded { peer: PeerId },
    /// A peer acknowledged a notice we sent it. Panel-only.
    NoticeDelivered { peer: PeerId },
    /// A runner answered our roster ask. `addr` is the address we last dialled
    /// it on, ending in `/p2p/<peer>`, for the panel's remembered players.
    /// Panel-only.
    RosterEntry { peer: PeerId, title_id: u32, display_name: String, addr: Option<String> },
    /// A reachable runner whose build has no notice protocol, so it cannot
    /// answer a roster ask or receive a notice. Panel-only.
    RosterOldBuild { peer: PeerId, title_id: u32 },
    /// The last connection to a peer reported by RosterEntry or RosterOldBuild
    /// closed. Panel-only.
    RosterGone { peer: PeerId },
    Error { message: String },
}

#[allow(dead_code)] // data_dir/display_name unread until friend/presence state lands
pub struct EngineConfig {
    pub data_dir: PathBuf,
    pub title_id: u32,
    pub display_name: String,
    /// Kademlia bootstrap multiaddrs (must end in /p2p/<peer>). Empty is
    /// valid: LAN mDNS, manual connect strings, and direct v6 still work.
    pub bootstrap: Vec<String>,
    /// Fixed QUIC listen port; 0 = ephemeral.
    pub listen_port: u16,
    /// Fixed game-plane UDP port; 0 = ephemeral.
    pub game_port: u16,
    /// Circuit-v2 relays to hold reservations on. Several are held at once
    /// rather than one being chosen: a reservation takes a round trip to
    /// establish, so discovering a dead relay at the moment you need it is
    /// too late.
    ///
    /// Accelerant, never authority (§9). An empty list is fully supported and
    /// a configured relay that never answers must cost nothing but speed.
    pub relays: Vec<String>,
    /// Skip hole punching and carry all game traffic over the control tunnel
    /// (§14 degraded path).
    ///
    /// A CGNAT or symmetric-NAT pair reaches this state on its own, but nobody
    /// developing on a LAN ever does -- every local punch succeeds, so the
    /// fallback goes untested until it fails for a real player who cannot
    /// report why. This makes that path reproducible on any machine.
    pub force_tunnel: bool,
    /// Public key notices and roster asks must be signed with:
    /// `notice::NOTICE_PUBLIC_KEY` outside tests. All zeros accepts nothing.
    pub notice_key: [u8; 32],
}

pub struct EngineHandles {
    pub diagnostics: Arc<crate::diagnostics::Recorder>,
    pub cmd_tx: mpsc::UnboundedSender<Command>,
    pub evt_rx: mpsc::UnboundedReceiver<Event>,
    /// Pinged on every event; `rexnet_wait_event` blocks on it.
    pub evt_notify: Arc<Notify>,
    /// Game-plane fast path, shared with the FFI send call.
    pub plane: Arc<GamePlane>,
    pub peer_id: PeerId,
    /// The notice slots; the FFI hands them to the runner's overlay.
    pub notices: Arc<std::sync::Mutex<crate::notice::Slots>>,
}

/// Echo payload for the milestone-1 connectivity proof.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct EchoPayload(Vec<u8>);

/// §10.2 session directory, carried over `/rexnet/session/1.0.0`.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct SessionQuery {
    schema: u8, // = 1
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct SessionDescMsg {
    schema: u8, // = 1
    session_id: [u8; 16],
    title_id: u32,
    slots_total: u8,
    slots_open: u8,
    requires_invite: bool,
    /// Title-defined bytes (XOnline attributes and QoS data). Defaults for
    /// peers that predate the field.
    #[serde(default)]
    data: Vec<u8>,
}

struct LocalSession {
    id: [u8; 16],
    slots_total: u8,
    slots_open: u8,
    is_public: bool,
    data: Vec<u8>,
}

/// Largest session data block; with the 16-byte id it fits one event.
pub(crate) const SESSION_DATA_MAX: usize = 1024;

/// What we serve a session query, built from our live session.
fn session_desc(session: &LocalSession, title_id: u32) -> SessionDescMsg {
    SessionDescMsg {
        schema: 1,
        session_id: session.id,
        title_id,
        slots_total: session.slots_total,
        slots_open: session.slots_open,
        requires_invite: !session.is_public,
        data: session.data.clone(),
    }
}

/// §8.1 presence, pushed over `/rexnet/presence/1.0.0` to mutual friends
/// on change and heartbeated. The transport (Noise/TLS) authenticates the
/// sender; the spec's detached signature only matters once records are
/// relayed/gossiped, so it is deferred until then.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct PresenceMsg {
    schema: u8, // = 1
    seq: u64,
    display_name: String,
    title_id: u32,
    state: u8, // 0 offline, 1 online, 2 in-game, 3 joinable
    rich: Vec<u8>,
}

/// §8.2 friendship, over `/rexnet/friend/1.0.0`.
#[derive(Debug, Clone, Serialize, Deserialize)]
enum FriendMsg {
    Request { schema: u8, display_name: String, note: String },
    Accept { schema: u8 },
    Remove { schema: u8 },
}

/// §8.3 invites, over `/rexnet/invite/1.0.0`. Offer and Reply are separate
/// one-shot messages (the human decision happens between them); Revoke
/// withdraws an offer.
#[derive(Debug, Clone, Serialize, Deserialize)]
enum InviteMsg {
    Offer {
        schema: u8,
        title_id: u32,
        session_id: [u8; 16],
        expires_unix: u64,
        #[serde(default)]
        note: String,
    },
    Reply { schema: u8, session_id: [u8; 16], accept: bool },
    Revoke { schema: u8, session_id: [u8; 16] },
}

/// Generic delivery ack for one-shot protocol messages.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct Ack {
    schema: u8,
}

/// Reply to a friend message. Carries the replier's display name, so the
/// sender of a request learns who it asked; the request names only an id.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct FriendAck {
    schema: u8,
    #[serde(default)]
    display_name: String,
}

/// A title message, carried over `/rexnet/message/1.0.0` and acknowledged
/// with `Ack`. The blob belongs to the title; `id` lets the receiver drop a
/// resend after a lost acknowledgement. `kind`: 0 title message, 1 club
/// request, 2 club handover.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct MessageEnvelope {
    schema: u8,
    id: [u8; 16],
    blob: Vec<u8>,
    #[serde(default)]
    kind: u8,
}

/// The club protocol: a member asks the owner for a newer record; the owner
/// pushes new versions and deletion.
#[derive(Debug, Clone, Serialize, Deserialize)]
enum ClubMsg {
    Query { schema: u8, team: u64, have_version: u64 },
    Push { schema: u8, team: u64, version: u64, record: Vec<u8>, members: Vec<Vec<u8>> },
    Deleted { schema: u8, team: u64 },
}

#[derive(Debug, Clone, PartialEq, Serialize, Deserialize)]
enum ClubReply {
    Record { version: u64, record: Vec<u8>, members: Vec<Vec<u8>> },
    Unchanged,
    NotMember,
    Deleted,
    Moved { owner: Vec<u8> },
    Unknown,
    Ack,
}

/// A club handed to a new owner: carried as a kind-2 envelope.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct Handover {
    team: u64,
    version: u64,
    record: Vec<u8>,
    members: Vec<Vec<u8>>,
}

const CLUB_NAME_CHECK_TIMEOUT: Duration = Duration::from_secs(10);

/// What to send a peer now: a pending club handover goes alone and first, so
/// the new owner holds the club before any request about it arrives.
fn deliverable(pending: Vec<([u8; 16], u8, Vec<u8>)>) -> Vec<([u8; 16], u8, Vec<u8>)> {
    if pending.iter().any(|(_, kind, _)| *kind == 2) {
        pending.into_iter().filter(|(_, kind, _)| *kind == 2).collect()
    } else {
        pending
    }
}

/// What we tell `peer`, who has `team` at `have_version`. Only the owner
/// answers with the record; anyone else who knows the owner says so.
fn club_reply(store: &crate::clubs::ClubStore, peer: &PeerId, team: u64, have_version: u64) -> ClubReply {
    use crate::clubs::Role;
    let Some(club) = store.get(team) else { return ClubReply::Unknown };
    match club.role {
        Role::Deleted => ClubReply::Deleted,
        Role::Member => match club.owner() {
            Some(owner) if owner != *peer => ClubReply::Moved { owner: owner.to_bytes() },
            _ => ClubReply::Unknown,
        },
        Role::Owner if !club.has_member(peer) => ClubReply::NotMember,
        Role::Owner if have_version >= club.version => ClubReply::Unchanged,
        Role::Owner => ClubReply::Record {
            version: club.version,
            record: club.record.clone(),
            members: club.members().iter().map(|p| p.to_bytes()).collect(),
        },
    }
}

const PRESENCE_HEARTBEAT: Duration = Duration::from_secs(30);

/// §8.4 punch signaling, carried over `/rexnet/punch/1.0.0`. Offer/answer
/// map onto request-response; probes then run on the game socket itself.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct PunchOffer {
    nonce: [u8; 16],
    candidates: Vec<SocketAddr>,
    /// X25519 public key for game-plane encryption (§6). Safe to send here:
    /// the control connection is already authenticated and confidential.
    eph_pub: [u8; 32],
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct PunchAnswer {
    nonce: [u8; 16],
    candidates: Vec<SocketAddr>,
    eph_pub: [u8; 32],
}

/// Ambient presence broadcast on a shard's gossipsub topic (§17.3.4).
///
/// Note what is **absent**: no display name. This reaches the whole shard,
/// most of whom are not friends, and §17.3.8 says a non-friend never learns
/// your self-asserted name. Receivers derive a pseudonym from the sender's
/// peer id instead; friends get the real name over the separate §8.1 push.
/// Adding a name field here would silently undo the privacy rule for every
/// title that enables shards.
#[derive(Debug, Clone, Serialize, Deserialize)]
struct ShardBeat {
    schema: u8, // = 1
    /// Same vocabulary as presence (§8.1): 0 offline, 1 online, 2 in-game,
    /// 3 joinable.
    state: u8,
    /// Per-title ambient KVs -- orb positions and the like. Opaque to core.
    rich: Vec<u8>,
    /// Set when hosting something joinable, so shard members can join in
    /// progress without first becoming friends.
    session_id: Option<[u8; 16]>,
}

/// §17.3.1 shard descriptor query over `/rexnet/shard/1.0.0`. Asked of a
/// peer that advertised itself as a shard provider; `None` means it no longer
/// holds one (records outlive membership).
#[derive(Debug, Clone, Serialize, Deserialize)]
struct ShardQuery {
    schema: u8, // = 1
    title_id: u32,
}

/// `/rexnet/notice/1.0.0`: a signed notice to keep and maybe pass on, or a
/// signed roster ask (docs/technical/rexnet-notices.md).
#[derive(Debug, Clone, Serialize, Deserialize)]
enum NoticeRequest {
    Notice(crate::notice::Signed),
    Ask(crate::notice::Signed),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
enum NoticeReply {
    Ack,
    /// None unless the ask was signed, fresh, and sent by its panel.
    Identity(Option<RosterIdentity>),
}

#[derive(Debug, Clone, Serialize, Deserialize)]
struct RosterIdentity {
    display_name: String,
    title_id: u32,
}

/// Ceiling on concurrently established connections.
///
/// The public DHT crawl is unbounded by nature: it dials whatever routing
/// wants, and we were holding hundreds of connections to IPFS peers with no
/// relationship to any game. That churn drowned the log and, more seriously,
/// exhausted the inbound substream allowance ("maximum number of inbound
/// substreams attempts has been exceeded") — a limit that is per-connection
/// but reached far sooner when thousands of connections are cycling.
///
/// The budget only has to cover what RexNet actually needs: a warm shard is
/// capped at 24 members (SHARD_WARM_CONNECT_LIMIT), plus friends, plus a
/// working DHT routing table. 192 leaves generous headroom for all of that
/// while stopping the crawl from growing without bound.
const MAX_ESTABLISHED_CONNECTIONS: u32 = 192;
/// Inbound is capped lower than the total: an unsolicited peer must never be
/// able to fill the table and crowd out the outbound dials we chose to make
/// (shard members, friends, a peer we are joining). Inbound connections from
/// non-RexNet peers are closed once identify shows they don't speak the shard
/// protocol, so public DHT peers can't exhaust the slots that players and the
/// notice panel need. Friends, wanted peers, configured relays and auto-relays
/// are exempt from that close.
const MAX_ESTABLISHED_INCOMING: u32 = 64;
/// How often the count of closed public inbound connections is logged.
const PUBLIC_INBOUND_LOG_INTERVAL: Duration = Duration::from_secs(60);

/// Messages from the per-stream pump tasks back into the engine.
///
/// The tasks own the socket halves and do the blocking work; the engine only
/// keeps the registry and turns these into guest-facing events. Stream ids are
/// allocated by the tasks from a shared counter so no round trip is needed to
/// name a stream that has only just arrived.
#[derive(Debug)]
enum StreamEvent {
    Opened {
        id: StreamId,
        peer: PeerId,
        local_port: u16,
        remote_port: u16,
        outbound: bool,
        writer: mpsc::UnboundedSender<Vec<u8>>,
    },
    Data { id: StreamId, data: Vec<u8> },
    Closed { id: StreamId },
    ConnectFailed { peer: PeerId, dst_port: u16, error: String },
}

/// Largest guest read handed over in one event; keeps a chatty stream from
/// producing unbounded event payloads.
const STREAM_READ_CHUNK: usize = 4096;

/// Drive one guest connection: header already exchanged, now shuttle bytes
/// until either side closes.
async fn pump_stream(
    id: StreamId,
    peer: PeerId, local_port: u16, remote_port: u16,
    mut stream: libp2p::Stream,
    mut outgoing: mpsc::UnboundedReceiver<Vec<u8>>,
    events: mpsc::UnboundedSender<StreamEvent>,
    diagnostics: Arc<crate::diagnostics::Recorder>,
) {
    use libp2p::futures::io::{AsyncReadExt, AsyncWriteExt};
    let mut buf = vec![0u8; STREAM_READ_CHUNK];
    loop {
        tokio::select! {
            // Guest -> peer.
            outbound = outgoing.recv() => {
                match outbound {
                    Some(bytes) => {
                        if let Err(error) = stream.write_all(&bytes).await {
                            tracing::warn!(stream_id = id, ?error, "guest TCP write failed");
                            diagnostics.incident("stream_write_failed",serde_json::json!({"peer":peer.to_base58(),"stream_id":id,"bytes":bytes.len(),"error":format!("{error:?}")}));
                            break;
                        }
                        diagnostics.record("stream_written", serde_json::json!({"peer":peer.to_base58(),
                            "stream_id":id,"src_port":local_port,"dst_port":remote_port,"bytes":bytes.len()}));
                        if let Err(error) = stream.flush().await {
                            tracing::warn!(stream_id = id, ?error, "guest TCP flush failed");
                            diagnostics.incident("stream_flush_failed",serde_json::json!({"peer":peer.to_base58(),"stream_id":id,"error":format!("{error:?}")}));
                            break;
                        }
                    }
                    // Sender dropped: the guest closed its socket.
                    None => break,
                }
            }
            // Peer -> guest.
            read = stream.read(&mut buf) => {
                match read {
                    Ok(0) => {
                        tracing::info!(stream_id = id, "guest TCP remote EOF");
                        diagnostics.record("stream_remote_eof",serde_json::json!({"peer":peer.to_base58(),"stream_id":id}));
                        break;
                    }
                    Ok(n) => {
                        diagnostics.record("stream_received", serde_json::json!({"peer":peer.to_base58(),
                            "stream_id":id,"src_port":remote_port,"dst_port":local_port,"bytes":n}));
                        if events
                            .send(StreamEvent::Data { id, data: buf[..n].to_vec() })
                            .is_err()
                        {
                            break;           // engine gone
                        }
                    }
                    Err(error) => {
                        tracing::warn!(stream_id = id, ?error, "guest TCP read failed");
                        diagnostics.incident("stream_read_failed",serde_json::json!({"peer":peer.to_base58(),"stream_id":id,"error":format!("{error:?}")}));
                        break;
                    },
                }
            }
        }
    }
    let _ = stream.close().await;
    let _ = events.send(StreamEvent::Closed { id });
}

/// Accept inbound guest TCP connections for the lifetime of the engine.
///
/// Each stream opens with a header naming the destination guest port, so the
/// header is read here before the connection is announced -- the shim cannot
/// match it to a listening socket without knowing the port.
fn spawn_stream_acceptor(
    mut control: crate::stream::Control,
    events: mpsc::UnboundedSender<StreamEvent>,
    next_id: Arc<AtomicU64>,
    diagnostics: Arc<crate::diagnostics::Recorder>,
) {
    let protocol = StreamProtocol::new(crate::protocol::TCP);
    let mut incoming = match control.accept(protocol) {
        Ok(incoming) => incoming,
        Err(err) => {
            tracing::error!(%err, "guest TCP: cannot accept streams");
            return;
        }
    };
    tokio::spawn(async move {
        use libp2p::futures::StreamExt;
        while let Some((peer, mut stream)) = incoming.next().await {
            let events = events.clone();
            let next_id = next_id.clone();
            let diagnostics = diagnostics.clone();
            tokio::spawn(async move {
                use libp2p::futures::io::AsyncReadExt;
                let mut header = [0u8; STREAM_HEADER_LEN];
                if stream.read_exact(&mut header).await.is_err() {
                    return;
                }
                let Some(header) = StreamHeader::decode(&header) else {
                    tracing::debug!(%peer, "guest TCP: bad stream header");
                    return;
                };
                let id = next_id.fetch_add(1, Ordering::Relaxed).max(1);
                let (writer_tx, writer_rx) = mpsc::unbounded_channel::<Vec<u8>>();
                // The initiator's dst_port is the port we are listening on;
                // its src_port is the far side from our point of view.
                if events
                    .send(StreamEvent::Opened {
                        id,
                        peer,
                        local_port: header.dst_port,
                        remote_port: header.src_port,
                        outbound: false,
                        writer: writer_tx,
                    })
                    .is_err()
                {
                    return;
                }
                pump_stream(id, peer, header.dst_port, header.src_port, stream, writer_rx, events, diagnostics).await;
            });
        }
    });
}

// --- Degraded game plane (§5, §14) ---
//
// One stream per peer per direction: we write on ours, read from theirs. Both
// sides degrade at once here, so a shared stream would need a tie-break.

/// Messages from the tunnel tasks back into the engine.
#[derive(Debug)]
enum TunnelEvent {
    FrameExpired { peer: PeerId, writer_id: u64 },
    WriterOpened { peer: PeerId, writer_id: u64 },
    /// A peer opened a tunnel to us: its punch failed.
    Opened { peer: PeerId },
    /// A datagram arrived over a peer's tunnel.
    Frame { peer: PeerId, src_port: u16, dst_port: u16, data: Vec<u8> },
    /// Our outbound tunnel to this peer went away (opened and later died, or
    /// never opened at all). Either way the writer is dead and a later send
    /// has to open a fresh one.
    WriterGone { peer: PeerId, writer_id: u64, error: Option<String> },
    ReaderClosed { peer: PeerId, error: String },
}

fn current_tunnel_writer(
    writers: &HashMap<PeerId, (u64, mpsc::Sender<(Instant, Vec<u8>)>)>, peer: &PeerId, writer_id: u64,
) -> bool {
    writers.get(peer).is_some_and(|(current, _)| *current == writer_id)
}

/// Bounded: the guest thinks it is sending UDP, so congestion should drop
/// rather than grow memory and latency.
const TUNNEL_WRITE_QUEUE: usize = 64;

/// Write queued frames to a peer's tunnel stream until it dies.
async fn pump_tunnel_writer(
    peer: PeerId,
    writer_id: u64,
    mut stream: libp2p::Stream,
    mut outgoing: mpsc::Receiver<(Instant, Vec<u8>)>,
    events: mpsc::UnboundedSender<TunnelEvent>,
    diagnostics: Arc<crate::diagnostics::Recorder>,
) {
    use libp2p::futures::io::AsyncWriteExt;
    let mut error = None;
    while let Some((expires, frame)) = outgoing.recv().await {
        if Instant::now() >= expires {
            let _ = events.send(TunnelEvent::FrameExpired { peer, writer_id });
            continue;
        }
        if let Err(err) = stream.write_all(&frame).await {
            error = Some(format!("write: {err:?}"));
            break;
        }
        // Per frame: a datagram held for company is a datagram delivered late.
        let src_port = u16::from_be_bytes([frame[2],frame[3]]);
        let dst_port = u16::from_be_bytes([frame[4],frame[5]]);
        diagnostics.record("tunnel_written",serde_json::json!({"peer":peer.to_base58(),
            "src_port":src_port,"dst_port":dst_port,"bytes":frame.len(),
            "payload_bytes":frame.len()-tunnel::TUNNEL_HEADER_LEN}));
        if let Err(err) = stream.flush().await {
            error = Some(format!("flush: {err:?}"));
            break;
        }
    }
    let _ = stream.close().await;
    let _ = events.send(TunnelEvent::WriterGone { peer, writer_id, error });
}

/// Open the outbound tunnel to `peer` and start writing to it.
fn spawn_tunnel_writer(
    mut control: crate::stream::Control,
    peer: PeerId,
    writer_id: u64,
    outgoing: mpsc::Receiver<(Instant, Vec<u8>)>,
    events: mpsc::UnboundedSender<TunnelEvent>,
    diagnostics: Arc<crate::diagnostics::Recorder>,
) {
    tokio::spawn(async move {
        let protocol = StreamProtocol::new(crate::protocol::TUNNEL);
        match control.open_stream(peer, protocol).await {
            Ok(stream) => {
                tracing::info!(%peer, "tunnel stream open (degraded game plane)");
                let _ = events.send(TunnelEvent::WriterOpened { peer, writer_id });
                pump_tunnel_writer(peer, writer_id, stream, outgoing, events, diagnostics).await;
            }
            Err(err) => {
                let _ = events.send(TunnelEvent::WriterGone {
                    peer,
                    writer_id,
                    error: Some(format!("open: {err:?}")),
                });
            }
        }
    });
}

/// Read framed datagrams from one peer's tunnel until it closes.
async fn pump_tunnel_reader(
    peer: PeerId,
    mut stream: libp2p::Stream,
    events: mpsc::UnboundedSender<TunnelEvent>,
    diagnostics: Arc<crate::diagnostics::Recorder>,
) {
    use libp2p::futures::io::{AsyncReadExt, AsyncWriteExt};
    let mut header = [0u8; tunnel::TUNNEL_HEADER_LEN];
    let mut failure = None;
    loop {
        if let Err(error) = stream.read_exact(&mut header).await {
            tracing::info!(%peer, ?error, "tunnel reader closed at frame header");
            failure = Some(format!("header: {error:?}"));
            break;
        }
        let Some((src_port, dst_port, len)) = tunnel::decode_header(&header) else {
            // Framing is lost and a byte stream cannot resynchronise.
            tracing::warn!(%peer, "tunnel: bad frame header, closing stream");
            failure = Some("bad_frame_header".into());
            break;
        };
        let mut data = vec![0u8; len];
        if let Err(error) = stream.read_exact(&mut data).await {
            tracing::warn!(%peer, expected_bytes = len, ?error, "tunnel reader closed during frame");
            failure = Some(format!("body (expected {len} bytes): {error:?}"));
            break;
        }
        diagnostics.record("tunnel_received",serde_json::json!({"peer":peer.to_base58(),
            "src_port":src_port,"dst_port":dst_port,"bytes":data.len(),"framed_bytes":data.len()+tunnel::TUNNEL_HEADER_LEN}));
        if events.send(TunnelEvent::Frame { peer, src_port, dst_port, data }).is_err() {
            break; // engine gone
        }
    }
    let _ = stream.close().await;
    if let Some(error) = failure { let _ = events.send(TunnelEvent::ReaderClosed {peer,error}); }
}

/// Accept inbound tunnels for the lifetime of the engine.
fn spawn_tunnel_acceptor(
    mut control: crate::stream::Control,
    events: mpsc::UnboundedSender<TunnelEvent>,
    diagnostics: Arc<crate::diagnostics::Recorder>,
) {
    let protocol = StreamProtocol::new(crate::protocol::TUNNEL);
    let mut incoming = match control.accept(protocol) {
        Ok(incoming) => incoming,
        Err(err) => {
            tracing::error!(%err, "tunnel: cannot accept streams");
            return;
        }
    };
    tokio::spawn(async move {
        use libp2p::futures::StreamExt;
        while let Some((peer, stream)) = incoming.next().await {
            tracing::info!(%peer, "peer opened a tunnel to us (their punch failed)");
            let _ = events.send(TunnelEvent::Opened { peer });
            tokio::spawn(pump_tunnel_reader(peer, stream, events.clone(), diagnostics.clone()));
        }
    });
}

#[derive(NetworkBehaviour)]
struct Behaviour {
    /// Must stay first: the derive polls fields in order, so the limiter sees
    /// (and can deny) a connection before other behaviours act on it.
    limits: connection_limits::Behaviour,
    identify: identify::Behaviour,
    kad: kad::Behaviour<MemoryStore>,
    mdns: mdns::tokio::Behaviour,
    autonat: autonat::Behaviour,
    /// Automatic NAT port mapping (§9 torrent-model reachability). On a
    /// UPnP/IGD router this opens the listen port unattended, so a peer becomes
    /// directly dialable without hand-forwarding anything -- the "seed if you
    /// can" half of the mesh.
    upnp: upnp::tokio::Behaviour,
    relay_client: relay::client::Behaviour,
    /// Circuit-v2 relay *server* (§9 torrent model): every node offers to relay
    /// for others, so a publicly reachable peer brokers rendezvous + DCUtR for
    /// NAT'd peers with no dedicated server. A NAT'd node still runs it, but no
    /// one can reach it to reserve, so relaying is self-selecting -- reachable
    /// peers seed, firewalled peers cannot, exactly like a torrent swarm.
    /// Reservation/data/time caps in `relay::Config` bound what we carry.
    relay_server: relay::Behaviour,
    dcutr: dcutr::Behaviour,
    echo: request_response::cbor::Behaviour<EchoPayload, EchoPayload>,
    punch: request_response::cbor::Behaviour<PunchOffer, PunchAnswer>,
    session: request_response::cbor::Behaviour<SessionQuery, Option<SessionDescMsg>>,
    presence: request_response::cbor::Behaviour<PresenceMsg, Ack>,
    friend: request_response::cbor::Behaviour<FriendMsg, FriendAck>,
    invite: request_response::cbor::Behaviour<InviteMsg, Ack>,
    message: request_response::cbor::Behaviour<MessageEnvelope, Ack>,
    club: request_response::cbor::Behaviour<ClubMsg, ClubReply>,
    shard: request_response::cbor::Behaviour<ShardQuery, Option<ShardDesc>>,
    notice: request_response::cbor::Behaviour<NoticeRequest, NoticeReply>,
    /// Guest TCP (§18) *and* the degraded game plane (§14). Raw libp2p
    /// streams rather than request-response protocols: a guest stream socket
    /// needs ordered bytes, and the tunnel needs a carrier whose cost does
    /// not scale with datagram rate.
    stream: crate::stream::Behaviour,
    /// Ambient shard broadcast (§17.3.4). Deliberately gossip rather than a
    /// mesh of direct links: 255 peers fully connected is ~32k connections.
    gossipsub: gossipsub::Behaviour,
}

/// Build the swarm and spawn the engine task onto the current tokio runtime.
/// Must be called from within a runtime context (the FFI wraps this in
/// `Runtime::block_on`).
pub async fn spawn(keypair: Keypair, config: EngineConfig) -> Result<EngineHandles, BoxError> {
    let peer_id = keypair.public().to_peer_id();
    let relay_peers = RelayPeers::default();
    let relay_limits = game_relay_config(relay_peers.clone());
    tracing::info!(%peer_id, title_id = config.title_id,
        retry_ms = LINK_MAINTENANCE_INTERVAL.as_millis(),
        recovery_grace_ms = PEER_RECOVERY_GRACE.as_millis(),
        recovery_retry_ms = PEER_RECOVERY_RETRY.as_millis(),
        route_cache_peers = ROUTE_CACHE_PEERS, route_cache_addresses = ROUTE_CACHE_ADDRS,
        quic_fallback_seconds = SHARD_RETRY_MAX.as_secs(),
        relay_circuit_seconds = relay_limits.max_circuit_duration.as_secs(),
        relay_circuit_bytes = relay_limits.max_circuit_bytes,
        "multiplayer recovery policy");

    let mut bandwidth_registry = libp2p::metrics::Registry::default();
    let mut swarm = libp2p::SwarmBuilder::with_existing_identity(keypair)
        .with_tokio()
        .with_tcp(
            libp2p::tcp::Config::default().nodelay(true),
            libp2p::noise::Config::new,
            libp2p::yamux::Config::default,
        )?
        .with_quic()
        .with_dns()?
        .with_relay_client(libp2p::noise::Config::new, libp2p::yamux::Config::default)?
        .with_bandwidth_metrics(&mut bandwidth_registry)
        .with_behaviour(|key, relay_client| {
            let local_id = key.public().to_peer_id();
            Ok(Behaviour {
                limits: connection_limits::Behaviour::new(
                    connection_limits::ConnectionLimits::default()
                        .with_max_established(Some(MAX_ESTABLISHED_CONNECTIONS))
                        .with_max_established_incoming(Some(MAX_ESTABLISHED_INCOMING))
                        // One connection per peer is all we ever need, and it
                        // stops a peer opening many in parallel.
                        .with_max_established_per_peer(Some(2)),
                ),
                identify: identify::Behaviour::new(
                    identify::Config::new("rexnet/0.1.0".into(), key.public())
                        .with_agent_version(format!("rexnet-core/{}", env!("CARGO_PKG_VERSION"))),
                ),
                // Amino-compatible protocol id so public-DHT FindPeer works
                // (spec §4). Client mode: we don't serve records for the
                // public DHT (server-mode etiquette is an open question, §16).
                kad: kad::Behaviour::with_config(
                    local_id,
                    MemoryStore::new(local_id),
                    kad::Config::new(kad::PROTOCOL_NAME),
                ),
                mdns: mdns::tokio::Behaviour::new(mdns::Config::default(), local_id)?,
                autonat: autonat::Behaviour::new(local_id, autonat::Config::default()),
                upnp: upnp::tokio::Behaviour::default(),
                relay_client,
                relay_server: relay::Behaviour::new(local_id, relay_limits),
                dcutr: dcutr::Behaviour::new(local_id),
                echo: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::ECHO),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                punch: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::PUNCH),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                session: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::SESSION),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                presence: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::PRESENCE),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                friend: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::FRIEND),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                invite: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::INVITE),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                message: request_response::cbor::Behaviour::new(
                    [(StreamProtocol::new(crate::protocol::MESSAGE), ProtocolSupport::Full)],
                    request_response::Config::default(),
                ),
                club: request_response::cbor::Behaviour::new(
                    [(StreamProtocol::new(crate::protocol::CLUB), ProtocolSupport::Full)],
                    request_response::Config::default(),
                ),
                shard: request_response::cbor::Behaviour::new(
                    [(
                        StreamProtocol::new(crate::protocol::SHARD),
                        ProtocolSupport::Full,
                    )],
                    request_response::Config::default(),
                ),
                notice: request_response::cbor::Behaviour::new(
                    [(StreamProtocol::new(crate::protocol::NOTICE), ProtocolSupport::Full)],
                    request_response::Config::default(),
                ),
                stream: crate::stream::Behaviour::new(),
                gossipsub: gossipsub::Behaviour::new(
                    gossipsub::MessageAuthenticity::Signed(key.clone()),
                    gossipsub::Config::default(),
                )
                .map_err(|err| err.to_string())?
                .with_metrics(&mut bandwidth_registry, gossipsub::MetricsConfig::default()),
            })
        })?
        .with_swarm_config(|c| c.with_idle_connection_timeout(Duration::from_secs(60)))
        .build();

    // On the public Amino DHT we stay in client mode (server-mode etiquette
    // is an open question, §16). With no public bootstrap — LAN or private
    // mesh — every node serves, so provider records work peer-to-peer.
    let kad_mode = if config.bootstrap.is_empty() {
        kad::Mode::Server
    } else {
        kad::Mode::Client
    };
    swarm.behaviour_mut().kad.set_mode(Some(kad_mode));

    let port = config.listen_port;
    for addr in [
        format!("/ip4/0.0.0.0/udp/{port}/quic-v1"),
        format!("/ip6/::/udp/{port}/quic-v1"),
        "/ip4/0.0.0.0/tcp/0".to_string(),
    ] {
        let addr: Multiaddr = addr.parse()?;
        if let Err(err) = swarm.listen_on(addr.clone()) {
            tracing::warn!(%addr, ?err, "listen failed");
        }
    }

    let mut bootstrap_peers = HashSet::new();
    for entry in &config.bootstrap {
        match entry.parse::<Multiaddr>() {
            Ok(addr) => match addr.iter().last() {
                Some(Protocol::P2p(peer)) => {
                    swarm.behaviour_mut().kad.add_address(&peer, addr.clone());
                    bootstrap_peers.insert(peer);
                }
                _ => tracing::warn!(%addr, "bootstrap addr lacks /p2p/ suffix, skipped"),
            },
            Err(err) => tracing::warn!(entry, %err, "bad bootstrap multiaddr, skipped"),
        }
    }
    if !config.bootstrap.is_empty() {
        if let Err(err) = swarm.behaviour_mut().kad.bootstrap() {
            tracing::warn!(%err, "kad bootstrap failed to start");
        }
    }

    // Deliberately non-fatal, all of it. A relay that is unparseable, down,
    // or withdrawn must not stop the engine starting -- that would make hosted
    // infrastructure a dependency, which is the thing §9 exists to prevent.
    let mut relay_addrs: Vec<(PeerId, Multiaddr)> = Vec::new();
    for entry in &config.relays {
        let addr: Multiaddr = match entry.parse() {
            Ok(addr) => addr,
            Err(err) => {
                tracing::warn!(entry, %err, "bad relay multiaddr, skipped");
                continue;
            }
        };
        let Some(Protocol::P2p(peer)) = addr.iter().last() else {
            // Without a peer id we cannot tell whether its reservation is
            // live, so it could never be retried or reported.
            tracing::warn!(%addr, "relay addr lacks /p2p/, skipped");
            continue;
        };
        if let Err(err) = swarm.listen_on(addr.clone().with(Protocol::P2pCircuit)) {
            tracing::warn!(%addr, %err, "relay reservation could not be started");
        }
        relay_addrs.push((peer, addr));
    }
    if !relay_addrs.is_empty() {
        tracing::info!(count = relay_addrs.len(), "relays configured");
    }

    // Game-plane socket: one UDP socket multiplexing every guest port; the
    // punch establishes one peer endpoint per remote (§6). A reader task
    // forwards inbound packets so the main loop can select! without
    // borrowing the socket.
    let game_socket =
        Arc::new(UdpSocket::bind(("0.0.0.0", config.game_port)).await?);
    tracing::info!(game_port = game_socket.local_addr()?.port(), "game socket bound");
    if !config.force_tunnel {
        tokio::spawn(map_game_port(game_socket.local_addr()?.port()));
    }
    let (cmd_tx, cmd_rx) = mpsc::unbounded_channel::<Command>();
    let (evt_tx, evt_rx) = mpsc::unbounded_channel::<Event>();
    let evt_notify = Arc::new(Notify::new());
    let emitter = Emitter::new(evt_tx, evt_notify.clone());
    let plane = Arc::new(GamePlane::new(game_socket.clone(), emitter.clone()));
    let diagnostics = crate::diagnostics::Recorder::start(&config.data_dir, config.title_id, &peer_id.to_base58());
    diagnostics.transport_registry(bandwidth_registry);
    plane.set_recorder(diagnostics.clone());

    // Ready peers' data frames are decrypted and delivered right here; only
    // probes and not-yet-mapped frames go through the engine.
    let (game_tx, game_rx) = mpsc::unbounded_channel::<(SocketAddr, Vec<u8>)>();
    {
        let socket = game_socket.clone();
        let plane = plane.clone();
        tokio::spawn(async move {
            // One bounded buffer covers any ordinary UDP wire datagram. The
            // old guest-payload-sized buffer omitted crypto/port overhead and
            // WSAEMSGSIZE silently terminated this task on valid large packets.
            let mut buf = vec![0u8; 65_536];
            loop {
                let (len,addr) = match socket.recv_from(&mut buf).await {
                    Ok(packet) => packet,
                    Err(error) => {
                        // UDP errors describe a datagram, not the lifetime of this
                        // shared socket. A failed probe to one departed peer must
                        // not stop receiving from every other player (WSA 10054).
                        if matches!(error.kind(), std::io::ErrorKind::ConnectionReset
                            | std::io::ErrorKind::ConnectionRefused | std::io::ErrorKind::Interrupted) {
                            if let Some(recorder) = plane.recorder() {
                                recorder.record("udp_receive_error",serde_json::json!({"error":format!("{error:?}"),
                                    "os_error":error.raw_os_error(),"receive_capacity":buf.len(),"recoverable":true}));
                            }
                            tracing::warn!(?error,"game UDP datagram receive error; reader continuing");
                            // Bound retries even if a platform repeatedly returns
                            // a stale error without waiting for another datagram.
                            tokio::time::sleep(Duration::from_millis(10)).await;
                            continue;
                        }
                        if let Some(recorder) = plane.recorder() {
                            recorder.incident("udp_reader_failed",serde_json::json!({"error":format!("{error:?}"),
                                "os_error":error.raw_os_error(),"receive_capacity":buf.len()}));
                        }
                        tracing::error!(?error,"game UDP reader stopped");
                        break;
                    }
                };
                if let Some(recorder) = plane.recorder() {
                    recorder.record("udp_wire_received",serde_json::json!({"wire_bytes":len,
                        "peer":plane.peer_at(&addr).map(|p|p.to_base58()),"endpoint":addr.to_string(),"frame_type":buf[..len].first().copied()}));
                }
                if plane.try_receive(addr, &buf[..len]) {
                    continue;
                }
                if game_tx.send((addr, buf[..len].to_vec())).is_err() {
                    if let Some(recorder) = plane.recorder() {
                        recorder.record("udp_reader_stopped",serde_json::json!({"reason":"engine_event_receiver_closed"}));
                    }
                    break;
                }
            }
        });
    }

    let friend_store = FriendStore::load(&config.data_dir);
    let mailbox = crate::mailbox::Mailbox::load(&config.data_dir);
    let clubs = crate::clubs::ClubStore::load(&config.data_dir);

    // Guest TCP (§18). The control opens outbound streams; the accept loop
    // below takes inbound ones. Both run outside the swarm task, so ids come
    // from a shared counter rather than a round trip.
    let stream_control = swarm.behaviour().stream.new_control();
    let (stream_tx, stream_rx) = mpsc::unbounded_channel::<StreamEvent>();
    let next_stream_id = Arc::new(AtomicU64::new(1));
    spawn_stream_acceptor(stream_control.clone(), stream_tx.clone(), next_stream_id.clone(), diagnostics.clone());

    // Degraded game plane (§14) rides the same stream behaviour; it accepts a
    // different protocol, so it needs its own control and accept loop.
    let tunnel_control = swarm.behaviour().stream.new_control();
    let (tunnel_tx, tunnel_rx) = mpsc::unbounded_channel::<TunnelEvent>();
    spawn_tunnel_acceptor(tunnel_control.clone(), tunnel_tx.clone(), diagnostics.clone());

    let notices = Arc::new(std::sync::Mutex::new(crate::notice::Slots::default()));
    tokio::spawn(
        Engine {
            diagnostics: diagnostics.clone(),
            swarm,
            emitter,
            plane: plane.clone(),
            game_socket,
            title_id: config.title_id,
            display_name: config.display_name.clone(),
            friend_store,
            connected: HashSet::new(),
            wanted_peers: HashSet::new(),
            announced: HashSet::new(),
            presence_seq: 0,
            my_presence: PresenceMsg {
                schema: 1,
                seq: 0,
                display_name: config.display_name.clone(),
                title_id: config.title_id,
                state: 0, // offline until the title publishes presence
                rich: Vec::new(),
            },
            announced_vip: None,
            invited: HashSet::new(),
            pending_invites: HashMap::new(),
            local_session: None,
            pending_session_search: None,
            relays: HashSet::new(),
            relay_addrs,
            relay_retry_at: HashMap::new(),
            rexnet_peers: HashSet::new(),
            relay_peers,
            shard_enabled: false,
            shard_cap: DEFAULT_SHARD_CAP,
            current_shard: None,
            shard_candidates: HashMap::new(),
            pending_shard_search: None,
            pending_shard_queries: HashMap::new(),
            shard_member_peers: HashSet::new(),
            shard_member_heard: HashMap::new(),
            shard_retry_at: HashMap::new(),
            shard_swept_at: None,
            last_shard_move: None,
            listen_addrs: Vec::new(),
            external_ips: Vec::new(),
            announced_external: HashSet::new(),
            auto_relays: HashSet::new(),
            bootstrap_peers,
            auto_relay_listeners: HashMap::new(),
            auto_relay_backoff: HashMap::new(),
            no_rexnet_relay_since: Instant::now(),
            confirmed_addrs: HashSet::new(),
            relay_provider_registered: false,
            pending_relay_search: None,
            vip_by_peer: HashMap::new(),
            peer_by_vip: HashMap::new(),
            pending_datagrams: HashMap::new(),
            orphan_frames: HashMap::new(),
            punch_deadline: HashMap::new(),
            streams: StreamTable::new(),
            stream_writers: HashMap::new(),
            stream_control,
            next_stream_id,
            stream_tx,
            force_tunnel: config.force_tunnel,
            tunneled_peers: HashSet::new(),
            peer_rtt_ms: HashMap::new(),
            tunnel_control,
            tunnel_writers: HashMap::new(),
            next_tunnel_writer: 0,
            tunnel_packets: HashMap::new(),
            tunnel_tx,
            tunnel_drops: HashMap::new(),
            peer_game_pubs: HashMap::new(),
            key_agreements: HashMap::new(),
            pending_punch_nonce: HashMap::new(),
            punch_replies: HashMap::new(),
            pending_punch_offers: HashMap::new(),
            pending_find: HashMap::new(),
            pending_echo: HashMap::new(),
            pending_keepalive: HashMap::new(),
            pending_friend_requests: HashMap::new(),
            friend_retry_at: HashMap::new(),
            peer_lookups: HashMap::new(),
            own_key_query: None,
            own_key_at: None,
            mailbox,
            messages_in_flight: HashMap::new(),
            clubs,
            club_queries: HashMap::new(),
            club_hops: HashMap::new(),
            club_key_queries: HashMap::new(),
            club_key_at: HashMap::new(),
            club_name_checks: HashMap::new(),
            circuit_limits: HashMap::new(),
            peer_links: HashMap::new(),
            peer_routes: HashMap::new(),
            recovering: HashMap::new(),
            public_inbound_closed: 0,
            tunnel_bytes: HashMap::new(),
            notice_key: config.notice_key,
            notices: notices.clone(),
            notice_peers: HashSet::new(),
            notice_targets: HashMap::new(),
            notice_sends: HashMap::new(),
            seed_searches: HashMap::new(),
            roster_searches: HashMap::new(),
            roster_asked: HashSet::new(),
            roster_asks: HashMap::new(),
            roster_ask: None,
            roster_seen: HashSet::new(),
            beacons: HashSet::new(),
        }
        .run(cmd_rx, game_rx, stream_rx, tunnel_rx),
    );

    tracing::info!(%peer_id, title_id = %format!("{:08X}", config.title_id), "rexnet engine up");
    Ok(EngineHandles { cmd_tx, evt_rx, evt_notify, plane, peer_id, notices, diagnostics })
}

struct Engine {
    diagnostics: Arc<crate::diagnostics::Recorder>,
    swarm: Swarm<Behaviour>,
    emitter: Emitter,
    /// Punched endpoints, session keys and the vip table live here so the
    /// fast paths can read them; this loop is their only writer.
    plane: Arc<GamePlane>,
    game_socket: Arc<UdpSocket>,
    title_id: u32,
    display_name: String,
    friend_store: FriendStore,
    connected: HashSet<PeerId>,
    /// Peers the app explicitly asked to connect to; only their dial
    /// failures surface as Event::Error — the kad crawl constantly fails
    /// against unreachable public peers and stays at tracing level.
    wanted_peers: HashSet<PeerId>,
    /// Peers surfaced to the app via PeerConnected. Gates the vip pool and
    /// the peer table to app-relevant peers; kad-crawl connections to
    /// random public DHT nodes never enter either.
    announced: HashSet<PeerId>,
    presence_seq: u64,
    my_presence: PresenceMsg,
    /// Last address handed to the shim, so re-announcing is free.
    announced_vip: Option<u32>,
    /// Peers we invited to the current session; they may fetch its
    /// descriptor even when the session is private.
    invited: HashSet<PeerId>,
    /// Invites waiting for a connection. An invite is usually the *first*
    /// thing we ever send a peer, so at that moment there is frequently no
    /// connection and often no known address either -- sending blind meant
    /// the first invite reliably failed and the user had to try twice.
    pending_invites: HashMap<PeerId, (InviteMsg, Instant)>,
    local_session: Option<LocalSession>,
    /// Active session-search provider query, with peers already asked.
    pending_session_search: Option<(kad::QueryId, Vec<PeerId>)>,

    // --- Ambient title shard, §17.3 -------------------------------------
    /// Peers currently carrying our traffic as a circuit relay (§9, §17.2).
    relays: HashSet<PeerId>,
    /// Configured relays and their addresses, for retrying a reservation that
    /// never came up or has since lapsed.
    relay_addrs: Vec<(PeerId, Multiaddr)>,
    /// When each relay was last dialled, so a dead one is retried slowly
    /// rather than hammered.
    relay_retry_at: HashMap<PeerId, Instant>,
    /// Connected peers whose identify shows they speak our shard protocol --
    /// i.e. other RexNet nodes, as opposed to the public DHT peers we are
    /// also connected to. Asked directly during shard discovery.
    rexnet_peers: HashSet<PeerId>,
    /// The same set, for the relay server's limiters on the swarm side.
    relay_peers: RelayPeers,
    /// Opt-in per title (§17.5); false means we never join or publish.
    shard_enabled: bool,
    shard_cap: u16,
    /// The shard we belong to, if any. We publish a provider record for it
    /// and are subscribed to its topic.
    current_shard: Option<ShardDesc>,
    /// Descriptors learned from the last sweep, keyed by shard id.
    shard_candidates: HashMap<[u8; 16], ShardDesc>,
    pending_shard_search: Option<(kad::QueryId, Vec<PeerId>)>,
    pending_shard_queries: HashMap<OutboundRequestId, PeerId>,
    /// Peer ids heard on our shard topic. Larger than the gossipsub mesh:
    /// these are everyone in the shard, not just who we relay through.
    shard_member_peers: HashSet<PeerId>,
    /// When each member last beat or subscribed; silence prunes them.
    shard_member_heard: HashMap<PeerId, Instant>,
    /// Next retry and current backoff; never permanently suppress a failed
    /// dial. A connection or disconnect resets it.
    shard_retry_at: HashMap<PeerId, (Instant, Duration)>,
    /// Set once a provider sweep has completed. Guards shard *creation*:
    /// without it the first rescan tick (which fires immediately) would
    /// create a shard before discovery had a chance to answer, so every node
    /// would found its own and the population would start fully fragmented.
    /// When the last provider sweep finished, or `None` if none has. Doubles
    /// as the swept flag so the two cannot drift apart.
    shard_swept_at: Option<Instant>,
    /// Migration rate limiting (§17.3.3). Direction is unconditional, so this
    /// is the only thing standing between a rescan and a stampede.
    last_shard_move: Option<Instant>,
    listen_addrs: Vec<Multiaddr>,
    external_ips: Vec<IpAddr>,
    /// Bare `host:port` endpoints already surfaced via [`Event::ExternalAddress`],
    /// so a re-confirmation of the same address does not re-emit.
    announced_external: HashSet<String>,
    /// Mesh peers we hold an automatic circuit reservation on (§9 torrent
    /// model), distinct from configured relays. Capped at [`MAX_AUTO_RELAYS`].
    auto_relays: HashSet<PeerId>,
    /// Peer ids of `config.bootstrap`, which are never auto relays.
    bootstrap_peers: HashSet<PeerId>,
    /// Each auto relay's circuit listener, with whether the relay is RexNet:
    /// a refused reservation closes the listener, and that frees its slot.
    auto_relay_listeners: HashMap<libp2p::core::transport::ListenerId, (PeerId, bool)>,
    /// Auto relays that refused or failed us, and when they may be picked again.
    auto_relay_backoff: HashMap<PeerId, Instant>,
    /// Since when we have held no RexNet auto relay: start, or losing the last one.
    no_rexnet_relay_since: Instant,
    /// External addresses confirmed so far. A beacon is announced again only
    /// for a new one: AutoNAT re-confirms the same address over and over.
    confirmed_addrs: HashSet<Multiaddr>,
    /// Whether we have published a `rexnet/v1/relays` provider record (§9). Set
    /// once we are confirmed reachable, so NAT'd peers can find us as a relay.
    relay_provider_registered: bool,
    /// In-flight `get_providers(rexnet/v1/relays)` query, so its result routes
    /// to relay handling rather than the session/shard registries.
    pending_relay_search: Option<kad::QueryId>,

    // Virtual-IP allocation (authoritative; the C++ shim mirrors it).
    vip_by_peer: HashMap<PeerId, u32>,
    peer_by_vip: HashMap<u32, PeerId>,

    // Punched game-plane endpoints.
    /// Datagrams waiting for a peer's punch to resolve, oldest first. Held
    /// unframed so they can leave by either the UDP game plane or the tunnel.
    pending_datagrams: HashMap<PeerId, Vec<PendingDatagram>>,
    /// Inbound frames from an endpoint we have not mapped to a peer yet,
    /// keyed by source address. Delivered once the punch completes.
    orphan_frames: HashMap<SocketAddr, Vec<(Instant, Vec<u8>)>>,
    /// When each in-flight punch stops being worth waiting for.
    punch_deadline: HashMap<PeerId, Instant>,
    /// Live guest TCP connections (§18).
    streams: StreamTable,
    /// Write half of each live stream, keyed the same way.
    stream_writers: HashMap<StreamId, mpsc::UnboundedSender<Vec<u8>>>,
    /// Handle used to open outbound streams; cloned into spawned tasks.
    stream_control: crate::stream::Control,
    /// Shared id allocator so a pump task can name a stream without a round
    /// trip back to the engine.
    next_stream_id: Arc<AtomicU64>,
    /// Pump tasks report here.
    stream_tx: mpsc::UnboundedSender<StreamEvent>,
    /// Punching disabled; everything takes the degraded path (§14).
    force_tunnel: bool,
    /// Peers whose punch failed and whose traffic now rides the control
    /// connection (§14). Cleared if a later punch (or DCUtR upgrade) succeeds.
    tunneled_peers: HashSet<PeerId>,
    /// Smoothed round-trip time per peer, milliseconds. Fed by probe acks and
    /// read by the shim for QoS, where titles show it as ping.
    peer_rtt_ms: HashMap<PeerId, u32>,
    /// Handle used to open outbound tunnel streams; cloned into the tasks.
    tunnel_control: crate::stream::Control,
    /// Write half of each peer's outbound tunnel. Bounded, because the guest
    /// thinks it is sending UDP and a backed-up link should drop, not grow.
    tunnel_writers: HashMap<PeerId, (u64, mpsc::Sender<(Instant, Vec<u8>)>)>,
    next_tunnel_writer: u64,
    tunnel_packets: HashMap<PeerId, (u64, u64)>,
    /// Tunnel tasks report here.
    tunnel_tx: mpsc::UnboundedSender<TunnelEvent>,
    /// Datagrams the tunnel dropped, per peer, since the last report. Only
    /// counted so loss can be seen in the log rather than guessed at from a
    /// title behaving oddly.
    tunnel_drops: HashMap<PeerId, u64>,
    /// The public key each peer's current game-plane keys were derived
    /// from. A different key in a later offer means the peer restarted and
    /// lost its state, and ours must follow or nothing authenticates until
    /// the stale connection times out.
    peer_game_pubs: HashMap<PeerId, [u8; 32]>,
    /// Our half of the game-plane key agreement, one per peer session (§6).
    /// Created when we first punch toward a peer and reused for every attempt,
    /// so simultaneous punches cannot derive mismatched keys.
    key_agreements: HashMap<PeerId, LocalKeyAgreement>,
    pending_punch_nonce: HashMap<[u8; 16], PeerId>,
    punch_replies: HashMap<PeerId, oneshot::Sender<Result<SocketAddr, String>>>,
    pending_punch_offers: HashMap<OutboundRequestId, PeerId>,

    pending_find: HashMap<kad::QueryId, (PeerId, oneshot::Sender<Result<Vec<Multiaddr>, String>>)>,
    pending_echo: HashMap<OutboundRequestId, (Instant, oneshot::Sender<Result<Duration, String>>)>,
    pending_keepalive: HashMap<OutboundRequestId, (PeerId, Instant)>,
    /// Friend requests in flight: the target, and whether the user just asked
    /// (its failure is reported) or it is a resend on reconnect.
    pending_friend_requests: HashMap<OutboundRequestId, (PeerId, bool)>,
    /// Backoff per stored peer we keep failing to reach.
    friend_retry_at: HashMap<PeerId, (Instant, Duration)>,
    /// DHT provider lookups for stored peers, by query.
    peer_lookups: HashMap<kad::QueryId, PeerId>,
    /// Our own reachability record: the publish in flight, and when one was
    /// last stored by a DHT node (None: publish again at the next heartbeat).
    own_key_query: Option<kad::QueryId>,
    own_key_at: Option<Instant>,
    /// Messages waiting for delivery and ids already received (`mailbox.bin`).
    mailbox: crate::mailbox::Mailbox,
    /// Message deliveries in flight: the recipient and message id.
    messages_in_flight: HashMap<OutboundRequestId, (PeerId, [u8; 16])>,
    /// Clubs we own or belong to (`clubs.bin`).
    clubs: crate::clubs::ClubStore,
    /// Our club queries in flight, by team.
    club_queries: HashMap<OutboundRequestId, u64>,
    /// `Moved` answers followed per team since its last record, to stop two
    /// stale holders sending us back and forth.
    club_hops: HashMap<u64, u32>,
    /// Our clubs' name-key publishes in flight, and when each last stored.
    club_key_queries: HashMap<kad::QueryId, u64>,
    club_key_at: HashMap<u64, Instant>,
    /// Name checks in flight: their id and when they started.
    club_name_checks: HashMap<kad::QueryId, (u32, Instant)>,
    /// Circuit budgets from the relay client, keyed by relay (outbound) or by
    /// the dialling peer (inbound); read when that connection is established.
    circuit_limits: HashMap<PeerId, (Option<Duration>, Option<u64>)>,
    /// Control connections per peer, oldest first.
    peer_links: HashMap<PeerId, Vec<Link>>,
    /// Short-lived, bounded routes survive libp2p routing-table eviction.
    peer_routes: HashMap<PeerId, PeerRoutes>,
    /// Preserve the game identity briefly while a failed transport recovers.
    recovering: HashMap<PeerId, PeerRecovery>,
    /// Inbound connections from non-RexNet peers closed since the last log line.
    public_inbound_closed: usize,
    /// Tunnel bytes per peer, both directions, never reset: the data half of
    /// a relayed link's budget is measured against it.
    tunnel_bytes: HashMap<PeerId, u64>,

    // --- Developer notices (docs/technical/rexnet-notices.md) -----------
    notice_key: [u8; 32],
    /// Shared with the FFI, so the runner reads them without the event queue.
    notices: Arc<std::sync::Mutex<crate::notice::Slots>>,
    /// Connected peers whose identify lists the notice protocol.
    notice_peers: HashSet<PeerId>,
    /// Targeted notices waiting for their peer's acknowledgement (panel).
    notice_targets: HashMap<PeerId, crate::notice::Signed>,
    /// Targeted notice requests in flight, by recipient.
    notice_sends: HashMap<OutboundRequestId, PeerId>,
    /// The panel's shard-key provider lookups, with their title: dial each
    /// provider (seed) or ask it who it is (roster). A seed search keeps the
    /// providers it has dialled, so a later kad step of the same query does not
    /// dial them again.
    seed_searches: HashMap<kad::QueryId, (u32, HashSet<PeerId>)>,
    roster_searches: HashMap<kad::QueryId, (u32, crate::notice::Signed)>,
    /// Peers asked in the current roster round (since the last `Command::Roster`).
    roster_asked: HashSet<PeerId>,
    /// Roster asks in flight, with their title: an `UnsupportedProtocols`
    /// failure of one is an old-build runner.
    roster_asks: HashMap<OutboundRequestId, u32>,
    /// The ask of the last `Command::Roster`: every RexNet peer that
    /// identifies from then on is asked with it at once. Never set on a runner.
    roster_ask: Option<(u32, crate::notice::Signed)>,
    /// Peers reported by RosterEntry or RosterOldBuild and still connected;
    /// each leaves with a RosterGone.
    roster_seen: HashSet<PeerId>,
    /// Titles whose shard key we provide without a shard (`Command::Beacon`).
    /// Never set on a runner.
    beacons: HashSet<u32>,
}

impl Engine {
    async fn run(
        mut self,
        mut cmd_rx: mpsc::UnboundedReceiver<Command>,
        mut game_rx: mpsc::UnboundedReceiver<(SocketAddr, Vec<u8>)>,
        mut stream_rx: mpsc::UnboundedReceiver<StreamEvent>,
        mut tunnel_rx: mpsc::UnboundedReceiver<TunnelEvent>,
    ) {
        // Bootstrap the shim's mirror from persisted state: friends and
        // requests either way, each with the name last heard.
        let stored: Vec<(PeerId, crate::friends::Entry)> =
            self.friend_store.entries().map(|(peer, entry)| (*peer, entry.clone())).collect();
        for (peer, entry) in stored {
            let display_name = entry.name;
            match entry.kind {
                Kind::Friend => self.emit(Event::FriendAccepted { peer, display_name }),
                Kind::In => self.emit(Event::FriendRequest { peer, display_name, note: entry.note }),
                Kind::Out => self.emit(Event::FriendRequestSent { peer, display_name }),
                Kind::Blocked => {}
            }
        }
        let clubs: Vec<crate::clubs::Club> = self.clubs.clubs().to_vec();
        for club in &clubs {
            self.emit_club(club);
        }
        self.announce_local_address();

        let mut heartbeat = tokio::time::interval(PRESENCE_HEARTBEAT);
        let mut shard_rescan = tokio::time::interval(SHARD_RESCAN_INTERVAL);
        let mut punch_watchdog = tokio::time::interval(PUNCH_WATCHDOG_INTERVAL);
        let mut shard_status = tokio::time::interval(SHARD_STATUS_INTERVAL);
        let mut diagnostic_tick = tokio::time::interval(Duration::from_secs(1));
        diagnostic_tick.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
        let mut last_diagnostic_tick = Instant::now();
        let mut relay_check = tokio::time::interval(RELAY_RETRY_INTERVAL);
        let mut link_maintenance = tokio::time::interval(LINK_MAINTENANCE_INTERVAL);
        let mut recovery_tick = tokio::time::interval(PEER_RECOVERY_TICK);
        recovery_tick.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
        let mut public_inbound_log = tokio::time::interval(PUBLIC_INBOUND_LOG_INTERVAL);
        let mut beacon_republish = tokio::time::interval(BEACON_REPUBLISH);
        // Once after a sleep, not once per missed period.
        beacon_republish.set_missed_tick_behavior(tokio::time::MissedTickBehavior::Skip);
        loop {
            tokio::select! {
                cmd = cmd_rx.recv() => {
                    let Some(cmd) = cmd else { break };
                    if !self.handle_command(cmd) {
                        break;
                    }
                }
                event = self.swarm.select_next_some() => self.handle_swarm_event(event),
                Some((addr, frame)) = game_rx.recv() => self.handle_game_frame(addr, frame),
                Some(event) = stream_rx.recv() => self.handle_stream_event(event),
                Some(event) = tunnel_rx.recv() => self.handle_tunnel_event(event),
                _ = heartbeat.tick() => {
                    self.push_presence(None);
                    self.shard_beat();
                    self.find_friends();
                    self.message_beat();
                    self.club_beat();
                }
                _ = link_maintenance.tick() => {
                    self.maintain_shard_links();
                    self.expire_club_name_checks();
                }
                _ = public_inbound_log.tick() => {
                    let closed = std::mem::take(&mut self.public_inbound_closed);
                    if closed > 0 {
                        tracing::info!("closed {closed} idle public inbound connections");
                    }
                }
                _ = punch_watchdog.tick() => self.check_punch_deadlines(),
                _ = recovery_tick.tick() => self.maintain_recoveries(),
                _ = beacon_republish.tick() => self.provide_beacons(),
                _ = shard_status.tick() => self.log_shard_status(),
                _ = diagnostic_tick.tick() => {
                    let gap = last_diagnostic_tick.elapsed().as_millis();
                    last_diagnostic_tick = Instant::now();
                    self.diagnostic_health(gap);
                }
                _ = relay_check.tick() => {
                    self.retry_dead_relays();
                    // Actively look for mesh relays in the DHT (§9) when short.
                    self.discover_relays();
                }
                _ = shard_rescan.tick() => {
                    // Sweep, then settle on the *next* tick once descriptors
                    // have arrived; shard_settle also runs when a sweep
                    // completes, so a slow DHT does not stall placement.
                    self.shard_settle();
                    self.shard_discover();
                }
            }
        }
        tracing::info!("rexnet engine down");
    }

    /// Record (or clear) a peer's relay role, announcing only real changes.
    /// DCUtR upgrades relayed connections to direct as soon as it can, so
    /// this flips more than once in a normal session.
    /// Re-dial configured relays with no live reservation.
    ///
    /// Reservations lapse when a relay restarts or the network drops, and
    /// nothing else notices: the node keeps working over direct paths, which
    /// is exactly why a silently dead relay would otherwise stay dead until
    /// the next launch.
    fn retry_dead_relays(&mut self) {
        let now = Instant::now();
        let dead: Vec<(PeerId, Multiaddr)> = self
            .relay_addrs
            .iter()
            .filter(|(peer, _)| !self.relays.contains(peer))
            .filter(|(peer, _)| self.relay_retry_at.get(peer).is_none_or(|at| now >= *at))
            .cloned()
            .collect();
        for (peer, addr) in dead {
            self.relay_retry_at.insert(peer, now + RELAY_RETRY_INTERVAL);
            match self.swarm.listen_on(addr.clone().with(Protocol::P2pCircuit)) {
                Ok(_) => tracing::debug!(%addr, "retrying relay reservation"),
                Err(err) => tracing::debug!(%addr, %err, "relay retry failed"),
            }
        }
    }

    fn mark_relay(&mut self, peer: PeerId, is_relay: bool) {
        let changed =
            if is_relay { self.relays.insert(peer) } else { self.relays.remove(&peer) };
        if !changed {
            return;
        }
        tracing::info!(
            %peer, is_relay,
            live = self.relays.len(), configured = self.relay_addrs.len(),
            "relay role changed"
        );
        // A relay is app-relevant even when it is nothing else to us: the UI
        // has to be able to show it.
        if is_relay {
            self.announce_peer(peer);
        }
        self.emit(Event::RelayStatus { peer, is_relay });
    }

    fn emit(&self, event: Event) {
        use serde_json::json;
        let metadata = match &event {
            Event::PeerConnected { peer, virtual_ip } => Some(("peer_connected",json!({"peer":peer.to_base58(),"virtual_ip":virtual_ip}))),
            Event::PeerDisconnected { peer } => Some(("peer_gone",json!({"peer":peer.to_base58()}))),
            Event::PunchResult { peer, ok } => Some(("punch_result",json!({"peer":peer.to_base58(),"ok":ok}))),
            Event::Degraded { peer } => Some(("tunnel_fallback",json!({"peer":peer.to_base58()}))),
            Event::RelayStatus { peer, is_relay } => Some(("relay_status",json!({"peer":peer.to_base58(),"is_relay":is_relay}))),
            Event::LocalAddress { virtual_ip } => Some(("local_address",json!({"virtual_ip":virtual_ip}))),
            Event::ExternalAddress { addr } => Some(("public_endpoint",json!({"endpoint":addr}))),
            Event::Error { message } => Some(("transport_error",json!({"message":message}))),
            _ => None,
        };
        if let Some((kind, fields)) = metadata { self.diagnostics.record(kind,fields); }
        self.emitter.emit(event);
    }

    // --- Ambient title shard (§17.3) ------------------------------------

    /// Live members: gossipsub topic peers plus ourselves. This is the
    /// authoritative count; `ShardDesc::members_hint` is a stale advisory for
    /// peers that have not subscribed yet (§17.3.4).
    fn shard_member_count(&self) -> u16 {
        let Some(current) = &self.current_shard else {
            return 0;
        };
        let topic = gossipsub::IdentTopic::new(crate::protocol::shard_topic(&current.shard_id));
        let hash = topic.hash();
        let peers = self
            .swarm
            .behaviour()
            .gossipsub
            .all_peers()
            .filter(|(_, topics)| topics.iter().any(|t| **t == hash))
            .count();
        u16::try_from(peers.saturating_add(1)).unwrap_or(u16::MAX)
    }

    /// Subscribe to a shard's topic and advertise ourselves as a provider so
    /// later arrivals can find it.
    fn shard_enter(&mut self, desc: ShardDesc) {
        let topic = gossipsub::IdentTopic::new(crate::protocol::shard_topic(&desc.shard_id));
        if let Err(err) = self.swarm.behaviour_mut().gossipsub.subscribe(&topic) {
            tracing::warn!(%err, "shard topic subscribe failed");
            return;
        }
        let key = kad::RecordKey::new(&crate::protocol::shard_key(self.title_id));
        if let Err(err) = self.swarm.behaviour_mut().kad.start_providing(key) {
            // Non-fatal: we are still in the shard, just harder to discover.
            tracing::warn!(%err, "shard provider record publish failed");
        }
        tracing::info!(
            shard = %hex16(&desc.shard_id), created_at = desc.created_at,
            "joined shard"
        );
        self.current_shard = Some(desc);
        self.readdress_all();
        // Peers already on the topic subscribed before we did, so no
        // Subscribed event will come for them.
        let hash = topic.hash();
        let members: Vec<PeerId> = self
            .swarm
            .behaviour()
            .gossipsub
            .all_peers()
            .filter(|(_, topics)| topics.iter().any(|t| **t == hash))
            .map(|(peer, _)| *peer)
            .collect();
        for peer in members {
            self.shard_member_seen(peer);
        }
    }

    /// A peer on our shard's topic is a member; `shard_member_count` counts
    /// exactly that. Take it on at its subscription or first beat, whichever
    /// comes first. Beats are 30 s apart, and until a peer is a member,
    /// subnet broadcasts skip it: System Link discovery (a title's search, a
    /// host's reply) went nowhere for up to half a minute after a join.
    fn shard_member_seen(&mut self, peer: PeerId) {
        // The public DHT crawl can fill the connection table; a member's redial
        // after a drop was denied there mid-race. Members are not counted.
        self.swarm.behaviour_mut().limits.bypass_peer_id(&peer);
        self.shard_member_heard.insert(peer, Instant::now());
        if self.shard_member_peers.insert(peer) {
            // A peer we already knew off-shard belongs on our /24 now, and
            // their arrival can shift placement for everyone, so re-derive
            // from one snapshot.
            self.readdress_all();
        }
        self.shard_ensure_reachable(peer);
        self.shard_warm_game_plane(peer);
    }

    /// Leave the current shard, unsubscribing and withdrawing our record.
    fn shard_leave(&mut self) {
        let Some(current) = self.current_shard.take() else {
            return;
        };
        let topic = gossipsub::IdentTopic::new(crate::protocol::shard_topic(&current.shard_id));
        let _ = self.swarm.behaviour_mut().gossipsub.unsubscribe(&topic);
        let key = kad::RecordKey::new(&crate::protocol::shard_key(self.title_id));
        self.swarm.behaviour_mut().kad.stop_providing(&key);
        for peer in self.shard_member_peers.drain() {
            self.swarm.behaviour_mut().limits.remove_peer_id(&peer);
        }
        self.shard_member_heard.clear();
        self.shard_retry_at.clear();
        tracing::info!(shard = %hex16(&current.shard_id), "left shard");
        self.readdress_all();
    }

    fn shard_create(&mut self) {
        let mut shard_id = [0u8; 16];
        rand::Rng::fill(&mut rand::thread_rng(), &mut shard_id);
        let created_at = std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .map(|d| d.as_secs())
            .unwrap_or(0);
        let desc = ShardDesc {
            schema: 1,
            shard_id,
            title_id: self.title_id,
            created_at,
            cap: self.shard_cap,
            members_hint: 1,
            anchor: self.swarm.local_peer_id().to_bytes(),
        };
        tracing::info!(shard = %hex16(&shard_id), "created shard");
        self.shard_enter(desc);
    }

    /// Ask one peer directly what shard it is in.
    fn shard_query_peer(&mut self, peer: PeerId) {
        tracing::info!(%peer, "querying peer for its shard");
        let title_id = self.title_id;
        let req = self
            .swarm
            .behaviour_mut()
            .shard
            .send_request(&peer, ShardQuery { schema: 1, title_id });
        self.pending_shard_queries.insert(req, peer);
    }

    /// Kick discovery: sweep the DHT *and* ask every RexNet peer we are
    /// already connected to.
    ///
    /// The direct half is not an optimisation. With a bootstrap configured we
    /// run kad in client mode (§4), so a node does not answer other peers'
    /// provider queries from its own store -- two instances on one LAN, already
    /// mDNS-connected, would otherwise have to round-trip their shard records
    /// through the public DHT and hope they propagate. Asking a peer we are
    /// holding a connection to is immediate and always works.
    fn shard_discover(&mut self) {
        if !self.shard_enabled {
            return;
        }
        let key = kad::RecordKey::new(&crate::protocol::shard_key(self.title_id));
        let id = self.swarm.behaviour_mut().kad.get_providers(key);
        self.pending_shard_search = Some((id, Vec::new()));

        let peers: Vec<PeerId> = self.rexnet_peers.iter().copied().collect();
        for peer in peers {
            self.shard_query_peer(peer);
        }
    }

    /// Apply §17.3.2 placement / §17.3.3 convergence to what we have learned.
    fn shard_settle(&mut self) {
        if !self.shard_enabled {
            return;
        }
        // Count friends per shard so affinity can outrank age. A shard we
        // know nothing about scores zero rather than being excluded -- an
        // unknown shard is still better than fragmenting into a new one.
        let candidates: Vec<Candidate> = self
            .shard_candidates
            .values()
            .map(|desc| Candidate {
                friends_present: self.shard_friend_count(&desc.shard_id),
                desc: desc.clone(),
            })
            .collect();

        match self.current_shard.clone() {
            None => {
                match shard::choose(self.title_id, &candidates) {
                    Placement::Join(id) => {
                        if let Some(desc) = self.shard_candidates.get(&id).cloned() {
                            self.shard_enter(desc);
                        }
                    }
                    Placement::Create => {
                        // Only found a shard once we have actually looked and
                        // every descriptor query has come back. Creating on
                        // incomplete knowledge is how a population fragments.
                        if !self.pending_shard_queries.is_empty() {
                            return;
                        }
                        match self.shard_swept_at {
                            None => {} // never looked
                            Some(at) if at.elapsed() >= SHARD_CREATE_GRACE => {
                                self.shard_create();
                            }
                            Some(_) => {
                                // A peer still connecting has not had the
                                // chance to answer yet (SHARD_CREATE_GRACE).
                                tracing::debug!("shard creation deferred; still looking");
                            }
                        }
                    }
                }
            }
            Some(current) => {
                // An Active session pins us in place: never relocate a player
                // mid-co-op to tidy up topology (§17.3.3).
                let session_active = self.local_session.is_some();
                if !self.shard_cooldown_elapsed() {
                    tracing::debug!("shard settle skipped (migration cooldown)");
                    return;
                }
                tracing::info!(
                    current = %hex16(&current.shard_id),
                    candidates = candidates.len(),
                    session_active,
                    "shard settle"
                );
                if let Migration::MoveTo(id) =
                    shard::should_migrate(self.title_id, &current, &candidates, session_active)
                {
                    // Roll per decision, not per node: a node that sits out
                    // this round re-decides next rescan, so the herd spreads
                    // across several intervals instead of one instant. Skipped
                    // for small shards, where there is no herd and the delay is
                    // pure cost.
                    let crowded = self.shard_member_peers.len() >= SHARD_JITTER_MIN_MEMBERS;
                    if crowded
                        && !rand::Rng::gen_bool(&mut rand::thread_rng(), SHARD_MIGRATION_CHANCE)
                    {
                        tracing::debug!("shard migration deferred (jitter)");
                        return;
                    }
                    if let Some(desc) = self.shard_candidates.get(&id).cloned() {
                        tracing::info!(
                            from = %hex16(&current.shard_id), to = %hex16(&id),
                            "migrating shard"
                        );
                        self.shard_leave();
                        self.shard_enter(desc);
                        self.last_shard_move = Some(Instant::now());
                    }
                }
            }
        }
    }

    /// Make a shard member reachable, so promoting them to Active (an invite
    /// or a join) does not have to start from nothing.
    ///
    /// Being in a shard is meant to mean "I can reach these people". Gossipsub
    /// alone does not deliver that: it meshes with a handful of peers and the
    /// rest are just peer ids arriving in messages. Small shards get a real
    /// connection each; large ones get an address resolved into the routing
    /// table, which is enough for a dial to succeed later.
    fn shard_ensure_reachable(&mut self, peer: PeerId) {
        if peer == *self.swarm.local_peer_id() || self.connected.contains(&peer)
            || self.recovering.contains_key(&peer) {
            return;
        }
        let small = self.shard_member_peers.len() <= SHARD_WARM_CONNECT_LIMIT;
        let (first, max) = if small { (LINK_MAINTENANCE_INTERVAL, SHARD_RETRY_MAX) }
            else { (SHARD_RESOLVE_INTERVAL, SHARD_RESOLVE_INTERVAL) };
        if !shard_retry_due(&mut self.shard_retry_at, peer, Instant::now(), first, max) {
            return;
        }
        if small {
            tracing::info!(%peer, "shard member: dialling to keep warm");
            let addresses = reconnect_routes(self.peer_routes.get(&peer), Instant::now());
            let dial = DialOpts::peer_id(peer).addresses(addresses).extend_addresses_through_behaviour().build();
            match self.swarm.dial(dial) {
                Ok(_) => return,
                Err(error) if shard_dial_needs_dht(&error) =>
                    tracing::info!(%peer, "shard dial has no address; resolving via DHT"),
                Err(error) => {
                    tracing::debug!(%peer, ?error, "shard dial not started");
                    return;
                }
            }
        }
        tracing::info!(%peer, "shard member: resolving address via DHT");
        self.swarm.behaviour_mut().kad.get_closest_peers(peer);
    }

    /// Bound maintenance to the same small-shard limit as eager punching.
    fn maintain_shard_links(&mut self) {
        let now = Instant::now();
        self.peer_routes.retain(|peer, routes| {
            if self.connected.contains(peer) { routes.updated = now; }
            now.saturating_duration_since(routes.updated) < ROUTE_CACHE_TTL
        });
        let departed = departed_members(&self.shard_member_heard, &self.connected, Instant::now());
        for peer in &departed {
            tracing::info!(%peer, "shard member went silent; dropping");
            self.swarm.behaviour_mut().limits.remove_peer_id(peer);
            self.shard_member_peers.remove(peer);
            self.shard_member_heard.remove(peer);
            self.shard_retry_at.remove(peer);
        }
        if !departed.is_empty() {
            self.readdress_all();
        }
        if self.shard_member_peers.len() > SHARD_WARM_CONNECT_LIMIT { return; }
        let peers: Vec<_> = self.shard_member_peers.iter().copied().collect();
        for peer in peers {
            self.shard_ensure_reachable(peer);
            self.shard_warm_game_plane(peer);
            if self.connected.contains(&peer)
                && !self.pending_keepalive.values().any(|(p, _)| *p == peer)
            {
                let id = self.swarm.behaviour_mut().echo.send_request(
                    &peer, EchoPayload(b"rexnet-keepalive".to_vec()));
                self.pending_keepalive.insert(id, (peer, Instant::now()));
            }
            // Reuse the punch probe and nonce to keep an idle UDP NAT mapping
            // alive too. It is protocol traffic, never a guest datagram.
            if let Some(endpoint) = self.ready_endpoint(&peer) {
                if let Some((nonce, _)) = self.pending_punch_nonce.iter().find(|(_, p)| **p == peer) {
                    let mut probe = Vec::with_capacity(PROBE_FRAME_LEN);
                    probe.push(FRAME_PROBE);
                    probe.extend_from_slice(nonce);
                    probe.extend_from_slice(&now_micros().to_be_bytes());
                    if let Err(error) = self.game_socket.try_send_to(&probe, endpoint) {
                        tracing::warn!(%peer, %endpoint, ?error, "game-plane keepalive send failed");
                    } else {
                        self.diagnostics.record("udp_probe_sent",serde_json::json!({"peer":peer.to_base58(),"wire_bytes":probe.len(),"reason":"keepalive"}));
                    }
                }
            }
            if self.connected.contains(&peer) && self.tunneled_peers.contains(&peer) {
                self.tunnel_writer(peer); // reopen a failed writer before the next guest packet
            }
            self.renew_relayed_link(peer);
        }
    }

    fn remember_peer_routes(&mut self, peer: PeerId, addresses: impl IntoIterator<Item = Multiaddr>) {
        remember_routes(&mut self.peer_routes, peer, addresses, Instant::now());
    }

    fn recover_peer(&mut self, peer: PeerId) {
        let now = Instant::now();
        let Some(recovery) = self.recovering.get_mut(&peer) else { return };
        if self.connected.contains(&peer) || now < recovery.next_dial { return; }
        recovery.next_dial = now + PEER_RECOVERY_RETRY;
        let addresses = reconnect_routes(self.peer_routes.get(&peer), now);
        let fields = serde_json::json!({"peer":peer.to_base58(),"addresses":addresses.iter().map(ToString::to_string).collect::<Vec<_>>(),
            "elapsed_ms":now.saturating_duration_since(recovery.started).as_millis()});
        self.diagnostics.record("game_peer_recovery_dial", fields);
        // Explicit alternatives must not be extended with the failed QUIC path.
        let dial = if addresses.is_empty() { DialOpts::peer_id(peer).condition(PeerCondition::NotDialing).build() }
            else { DialOpts::peer_id(peer).condition(PeerCondition::NotDialing).addresses(addresses).build() };
        if let Err(error) = self.swarm.dial(dial) {
            if shard_dial_needs_dht(&error) {
                self.lookup_recovery_peer(peer);
            }
            tracing::debug!(%peer, ?error, "recovery dial not started");
        }
    }

    fn lookup_recovery_peer(&mut self, peer: PeerId) {
        let Some(recovery) = self.recovering.get_mut(&peer) else { return };
        if recovery.lookup.is_none() {
            recovery.lookup = Some(self.swarm.behaviour_mut().kad.get_closest_peers(peer));
            self.diagnostics.record("game_peer_recovery_lookup", serde_json::json!({"peer":peer.to_base58()}));
        }
    }

    fn maintain_recoveries(&mut self) {
        let now = Instant::now();
        let peers: Vec<_> = self.recovering.keys().copied().collect();
        for peer in peers {
            let recovery = &self.recovering[&peer];
            if now.saturating_duration_since(recovery.started) >= PEER_RECOVERY_GRACE {
                self.expire_peer_recovery(peer);
            } else {
                self.recover_peer(peer);
            }
        }
    }

    fn game_path_recovered(&mut self, peer: PeerId, path: &str) {
        if !self.connected.contains(&peer) { return; }
        if self.recovering.get(&peer).is_some_and(|r| r.started.elapsed() >= PEER_RECOVERY_GRACE) {
            self.expire_peer_recovery(peer);
            return;
        }
        if let Some(recovery) = self.recovering.remove(&peer) {
            self.diagnostics.record("game_peer_recovered", serde_json::json!({"peer":peer.to_base58(),"path":path,
                "elapsed_ms":recovery.started.elapsed().as_millis()}));
            tracing::info!(%peer, path, elapsed_ms = recovery.started.elapsed().as_millis(), "game path recovered");
        }
    }

    fn expire_peer_recovery(&mut self, peer: PeerId) {
        if let Some(recovery) = self.recovering.remove(&peer) {
            self.diagnostics.incident("game_peer_recovery_expired", serde_json::json!({"peer":peer.to_base58(),
                "elapsed_ms":recovery.started.elapsed().as_millis(),"control_connected":self.connected.contains(&peer)}));
            self.pending_datagrams.remove(&peer);
            if let Some(reply) = self.punch_replies.remove(&peer) {
                let _ = reply.send(Err("peer recovery timed out".into()));
            }
            self.report_peer_departure(peer);
        }
    }

    fn report_peer_departure(&mut self, peer: PeerId) {
        if self.announced.remove(&peer) { self.emit(Event::PeerDisconnected { peer }); }
        if self.roster_seen.remove(&peer) { self.emit(Event::RosterGone { peer }); }
    }

    /// A relay ends a circuit at its time or data budget, and the tunnel on it
    /// goes too: a public relay's 120 s cap ended an MC3 race. Dial a
    /// replacement at half budget, which may also find a direct path; its
    /// arrival retires this link (`retire_superseded_links`).
    fn renew_relayed_link(&mut self, peer: PeerId) {
        let bytes = self.tunnel_bytes.get(&peer).copied().unwrap_or(0);
        let Some(links) = self.peer_links.get(&peer) else { return };
        if !renewal_due(links, Instant::now(), bytes) {
            return;
        }
        tracing::info!(%peer, tunnel_bytes = bytes, "relayed link at half its circuit budget; dialling a replacement");
        let dial = DialOpts::peer_id(peer).condition(PeerCondition::NotDialing).build();
        if let Err(error) = self.swarm.dial(dial) {
            tracing::info!(%peer, ?error, "replacement dial could not start");
        }
    }

    fn retire_superseded_links(&mut self, peer: PeerId) {
        let Some(links) = self.peer_links.get(&peer) else { return };
        // Both ends may dial a replacement at once and see the links arrive in
        // different orders; each keeping its own newest could close them all.
        // Between relayed links only the lower peer id chooses.
        if links.iter().all(|link| link.relayed) && *self.swarm.local_peer_id() > peer {
            return;
        }
        for id in superseded(links) {
            if self.swarm.close_connection(id) {
                tracing::info!(%peer, connection_id = %id, "closing superseded relayed link; the tunnel reopens on the newer one");
            }
        }
    }

    /// Close the inbound links of a peer identify showed is not a RexNet node.
    /// Public DHT peers serve us nothing and would hold MAX_ESTABLISHED_INCOMING
    /// slots that players and the notice panel dial into. Our own dials (kad,
    /// relays, bootstrap) are outbound and stay.
    fn close_public_inbound(&mut self, peer: PeerId) {
        if self.wanted_peers.contains(&peer)
            || self.auto_relays.contains(&peer)
            || self.relay_addrs.iter().any(|(relay, _)| *relay == peer)
            || self.friend_store.is_friend(&peer)
        {
            return;
        }
        for link in self.peer_links.get(&peer).into_iter().flatten().filter(|link| link.inbound) {
            if self.swarm.close_connection(link.id) {
                self.public_inbound_closed += 1;
            }
        }
    }

    /// Punch the game plane to a shard member ahead of time.
    ///
    /// A control connection alone does not make a peer playable: the game
    /// plane is a separate punched UDP path, and establishing it on demand is
    /// what makes the first co-op invite race (both sides punch independently,
    /// and whoever finishes first sends into an endpoint the other has not
    /// mapped yet). Being in a shard is supposed to mean "ready to play", so
    /// members of a small shard get their game plane punched up front and an
    /// invite finds it already there.
    ///
    /// Only for shards within the warm limit: punching 254 members would be
    /// both pointless and hostile to NAT tables.
    fn shard_warm_game_plane(&mut self, peer: PeerId) {
        if self.shard_member_peers.len() > SHARD_WARM_CONNECT_LIMIT {
            return;
        }
        if self.game_plane_ready(&peer)
            || self.punch_deadline.contains_key(&peer)
            || self.tunneled_peers.contains(&peer)
        {
            return; // already punched, punching, or knowingly degraded
        }
        if !self.connected.contains(&peer) {
            return; // the punch offer needs a control connection to ride
        }
        tracing::info!(%peer, "warming game plane for shard member");
        self.begin_punch(peer);
    }

    /// Broadcast our ambient state to the shard (§17.3.4). Cheap and
    /// low-rate: this is the only traffic a Connected-tier peer generates for
    /// players it never interacts with.
    fn shard_beat(&mut self) {
        let Some(current) = self.current_shard.clone() else {
            return;
        };
        let beat = ShardBeat {
            schema: 1,
            state: self.my_presence.state,
            rich: self.my_presence.rich.clone(),
            session_id: self.local_session.as_ref().map(|s| s.id),
        };
        let Ok(bytes) = postcard::to_allocvec(&beat) else {
            return;
        };
        let topic = gossipsub::IdentTopic::new(crate::protocol::shard_topic(&current.shard_id));
        if let Err(err) = self.swarm.behaviour_mut().gossipsub.publish(topic, bytes) {
            // InsufficientPeers is the normal case for a shard of one; it is
            // not worth a warning every heartbeat.
            match err {
                gossipsub::PublishError::NoPeersSubscribedToTopic => {
                    tracing::trace!("shard beat: no topic peers yet");
                }
                other => tracing::debug!(err = %other, "shard beat publish failed"),
            }
        }
    }

    /// Migration cooldown. Direction is unconditional by design (§17.3.3), so
    /// without this every member of a younger shard would move at once.
    fn shard_cooldown_elapsed(&self) -> bool {
        match self.last_shard_move {
            None => true,
            Some(at) => at.elapsed() >= SHARD_MIGRATION_COOLDOWN,
        }
    }

    /// Friends currently subscribed to a shard's topic.
    fn shard_friend_count(&self, shard_id: &[u8; 16]) -> u16 {
        let topic = gossipsub::IdentTopic::new(crate::protocol::shard_topic(shard_id));
        let hash = topic.hash();
        let count = self
            .swarm
            .behaviour()
            .gossipsub
            .all_peers()
            .filter(|(peer, topics)| {
                self.friend_store.is_friend(peer) && topics.iter().any(|t| **t == hash)
            })
            .count();
        u16::try_from(count).unwrap_or(u16::MAX)
    }



    /// Push our presence record to one connected friend or club member, or
    /// to all of them (§8.1: pushed on change, heartbeated every 30 s).
    fn push_presence(&mut self, only: Option<PeerId>) {
        self.presence_seq += 1;
        self.my_presence.seq = self.presence_seq;
        let msg = self.my_presence.clone();
        let local = *self.swarm.local_peer_id();
        let targets: Vec<PeerId> = match only {
            Some(peer) => vec![peer],
            None => {
                let mut all: HashSet<PeerId> = self.friend_store.friends().copied().collect();
                all.extend(self.clubs.peers(&local));
                all.into_iter().filter(|p| self.connected.contains(p)).collect()
            }
        };
        for peer in targets {
            if self.friend_store.is_friend(&peer) || self.clubs.knows(&peer, &local) {
                self.swarm.behaviour_mut().presence.send_request(&peer, msg.clone());
            }
        }
    }

    /// Send our pending request to `peer`. `asked` marks one the user just
    /// made (a failure drops it), as opposed to a resend on reconnect.
    fn send_friend_request(&mut self, peer: PeerId, asked: bool) {
        let note = self.friend_store.get(&peer).map(|e| e.note.clone()).unwrap_or_default();
        let msg = FriendMsg::Request { schema: 1, display_name: self.display_name.clone(), note };
        let id = self.swarm.behaviour_mut().friend.send_request(&peer, msg);
        self.pending_friend_requests.insert(id, (peer, asked));
    }

    /// Advertise our own key, then look up and dial the stored peers we are
    /// not connected to, so friends meet wherever they are online.
    fn find_friends(&mut self) {
        let local = *self.swarm.local_peer_id();
        if self.own_key_query.is_none() && own_key_due(self.own_key_at, Instant::now()) {
            let key = kad::RecordKey::new(&crate::protocol::peer_key(&local));
            match self.swarm.behaviour_mut().kad.start_providing(key) {
                Ok(id) => self.own_key_query = Some(id),
                Err(err) => tracing::warn!(%err, "own peer record publish failed"),
            }
        }
        let now = Instant::now();
        let mut extra = self.mailbox.peers();
        extra.extend(self.clubs.peers(&local));
        for peer in lookup_targets(&self.friend_store, &extra, &self.connected) {
            if self.peer_lookups.values().any(|target| *target == peer) { continue; }
            if !shard_retry_due(&mut self.friend_retry_at, peer, now, FRIEND_LOOKUP_FIRST, FRIEND_LOOKUP_MAX) {
                continue;
            }
            tracing::info!(%peer, "looking up friend");
            self.wanted_peers.insert(peer);
            let _ = self.swarm.dial(peer);
            let key = kad::RecordKey::new(&crate::protocol::peer_key(&peer));
            let id = self.swarm.behaviour_mut().kad.get_providers(key);
            self.peer_lookups.insert(id, peer);
        }
    }

    /// Send every message waiting for `peer` that is not already on its way.
    fn deliver_messages(&mut self, peer: PeerId) {
        for (id, kind, blob) in deliverable(self.mailbox.pending_kinds_for(&peer)) {
            if self.messages_in_flight.values().any(|(p, i)| *p == peer && *i == id) {
                continue;
            }
            let request = self.swarm.behaviour_mut().message.send_request(&peer, MessageEnvelope { schema: 1, id, kind, blob });
            self.messages_in_flight.insert(request, (peer, id));
        }
    }

    /// Keep `blob` for `peer` until acknowledged, and send it if we can.
    fn queue_envelope(&mut self, peer: PeerId, id: [u8; 16], kind: u8, blob: Vec<u8>) {
        if blob.len() > crate::mailbox::MAX_BLOB {
            self.emit(Event::Error { message: "message over 64 KB dropped".into() });
            return;
        }
        self.mailbox.push_kind(peer, id, kind, blob, crate::mailbox::unix_now());
        if self.connected.contains(&peer) {
            self.deliver_messages(peer);
        } else {
            // Found by the next heartbeat's lookups if this dial has no address.
            self.wanted_peers.insert(peer);
            let _ = self.swarm.dial(peer);
        }
    }

    /// A club handed to us by its owner.
    fn receive_handover(&mut self, peer: PeerId, blob: &[u8]) {
        let Ok(handover) = postcard::from_bytes::<Handover>(blob) else {
            tracing::info!(%peer, "malformed club handover dropped");
            return;
        };
        // Only the club's current owner may hand it over.
        if !self.clubs.get(handover.team).is_some_and(|c| c.owner() == Some(peer)) {
            tracing::info!(%peer, team = handover.team, "club handover from a non-owner dropped");
            return;
        }
        let local = *self.swarm.local_peer_id();
        let members: Vec<PeerId> =
            handover.members.iter().filter_map(|m| PeerId::from_bytes(m).ok()).filter(|p| *p != local).collect();
        let Handover { team, version, record, .. } = handover;
        if !self.clubs.set_owned(team, &local, version, record.clone(), &members, None, crate::mailbox::unix_now()) {
            return;
        }
        tracing::info!(%peer, team, "club handed over to us");
        self.emit(Event::ClubUpdated { team, owner: local, version, owned: true, record, members });
    }

    /// Provide each owned club's name key when due: at once, then every ten minutes.
    fn advertise_club_names(&mut self) {
        let now = Instant::now();
        let due: Vec<(u64, String)> = self
            .clubs
            .clubs()
            .iter()
            .filter(|c| c.role == crate::clubs::Role::Owner)
            .filter_map(|c| Some((c.team, c.name_key.clone()?)))
            .filter(|(team, _)| {
                own_key_due(self.club_key_at.get(team).copied(), now) && !self.club_key_queries.values().any(|t| t == team)
            })
            .collect();
        for (team, key) in due {
            match self.swarm.behaviour_mut().kad.start_providing(kad::RecordKey::new(&key)) {
                Ok(query) => {
                    self.club_key_queries.insert(query, team);
                }
                Err(err) => tracing::warn!(%err, team, "club name publish failed"),
            }
        }
    }

    /// Stop providing `team`'s name key.
    fn withdraw_club_name(&mut self, team: u64) {
        if let Some(key) = self.clubs.get(team).and_then(|c| c.name_key.clone()) {
            self.swarm.behaviour_mut().kad.stop_providing(&kad::RecordKey::new(&key));
        }
        self.club_key_at.remove(&team);
    }

    /// A name check that has heard nothing for 10 s reports the name free.
    fn expire_club_name_checks(&mut self) {
        let expired: Vec<(kad::QueryId, u32)> = self
            .club_name_checks
            .iter()
            .filter(|(_, (_, at))| at.elapsed() >= CLUB_NAME_CHECK_TIMEOUT)
            .map(|(query, (id, _))| (*query, *id))
            .collect();
        for (query, id) in expired {
            self.club_name_checks.remove(&query);
            self.emit(Event::ClubNameResult { id, taken: false });
        }
    }

    /// Heartbeat: expire old mail, and retry deliveries to connected peers.
    fn message_beat(&mut self) {
        self.mailbox.expire(crate::mailbox::unix_now());
        for peer in self.mailbox.peers() {
            if self.connected.contains(&peer) {
                self.deliver_messages(peer);
            }
        }
    }

    fn emit_club(&self, club: &crate::clubs::Club) {
        use crate::clubs::Role;
        let Some(owner) = club.owner().filter(|_| club.role != Role::Deleted) else { return };
        self.emit(Event::ClubUpdated {
            team: club.team,
            owner,
            version: club.version,
            owned: club.role == Role::Owner,
            record: club.record.clone(),
            members: club.members(),
        });
    }

    /// Heartbeat: expire deleted markers and clubs never confirmed.
    fn club_beat(&mut self) {
        for team in self.clubs.expire(crate::mailbox::unix_now()) {
            self.emit(Event::ClubGone { team, deleted: false });
        }
        self.advertise_club_names();
    }

    /// Ask the owner of a club we belong to for a newer record.
    fn query_club(&mut self, team: u64) {
        let Some(club) = self.clubs.get(team).filter(|c| c.role == crate::clubs::Role::Member) else { return };
        let (Some(owner), have_version) = (club.owner(), club.version) else { return };
        if !self.connected.contains(&owner) {
            // Queried when the connection is up (ConnectionEstablished).
            self.wanted_peers.insert(owner);
            let _ = self.swarm.dial(owner);
            return;
        }
        let id = self.swarm.behaviour_mut().club.send_request(&owner, ClubMsg::Query { schema: 1, team, have_version });
        self.club_queries.insert(id, team);
    }

    /// A club message from `peer`: a member's query, or the owner's push or deletion.
    fn handle_club_msg(&mut self, peer: PeerId, msg: ClubMsg) -> ClubReply {
        use crate::clubs::Role;
        let local = *self.swarm.local_peer_id();
        let from_owner = |store: &crate::clubs::ClubStore, team: u64| {
            store.get(team).is_some_and(|c| c.role == Role::Member && c.owner() == Some(peer))
        };
        match msg {
            ClubMsg::Query { team, have_version, .. } => {
                self.announce_peer(peer);
                club_reply(&self.clubs, &peer, team, have_version)
            }
            ClubMsg::Push { team, version, record, members, .. } => {
                if !from_owner(&self.clubs, team) {
                    // The club may have changed hands while we stayed connected
                    // to its old owner: ask the owner we know, who sends us on.
                    if self.clubs.get(team).is_some_and(|c| c.role == Role::Member) {
                        self.query_club(team);
                    }
                    tracing::info!(%peer, team, "club push from a non-owner ignored");
                    return ClubReply::Ack;
                }
                if self.clubs.get(team).is_some_and(|c| c.version >= version) {
                    return ClubReply::Ack;
                }
                let members: Vec<PeerId> = members.iter().filter_map(|m| PeerId::from_bytes(m).ok()).collect();
                if !members.contains(&local) {
                    self.clubs.forget(team);
                    self.emit(Event::ClubGone { team, deleted: false });
                } else if self.clubs.set_member(team, &peer, version, record.clone(), &members, crate::mailbox::unix_now()) {
                    self.club_hops.remove(&team);
                    self.emit(Event::ClubUpdated { team, owner: peer, version, owned: false, record, members });
                }
                ClubReply::Ack
            }
            ClubMsg::Deleted { team, .. } => {
                if from_owner(&self.clubs, team) {
                    self.clubs.forget(team);
                    self.emit(Event::ClubGone { team, deleted: true });
                }
                ClubReply::Ack
            }
        }
    }

    /// The answer to our query about `team`.
    fn handle_club_reply(&mut self, peer: PeerId, team: u64, reply: ClubReply) {
        let local = *self.swarm.local_peer_id();
        let Some(club) = self.clubs.get(team) else { return };
        let provisional = club.version == 0;
        match reply {
            ClubReply::Record { version, record, members } => {
                let members: Vec<PeerId> = members.iter().filter_map(|m| PeerId::from_bytes(m).ok()).collect();
                if members.contains(&local)
                    && self.clubs.set_member(team, &peer, version, record.clone(), &members, crate::mailbox::unix_now())
                {
                    self.club_hops.remove(&team);
                    self.emit(Event::ClubUpdated { team, owner: peer, version, owned: false, record, members });
                }
            }
            // A provisional club waits: the owner may not have our answer yet.
            ClubReply::NotMember if !provisional => {
                self.clubs.forget(team);
                self.emit(Event::ClubGone { team, deleted: false });
            }
            ClubReply::Deleted => {
                self.clubs.forget(team);
                self.emit(Event::ClubGone { team, deleted: true });
            }
            ClubReply::Moved { owner } => {
                let hops = self.club_hops.entry(team).or_insert(0);
                *hops += 1;
                match PeerId::from_bytes(&owner) {
                    Ok(owner) if owner != local && *hops <= 3 => {
                        tracing::info!(team, %owner, "club moved; asking its new owner");
                        self.clubs.set_owner(team, &owner);
                        self.query_club(team);
                    }
                    _ => tracing::info!(team, "club move not followed"),
                }
            }
            _ => {}
        }
    }

    fn handle_friend_msg(&mut self, peer: PeerId, msg: FriendMsg) {
        match msg {
            FriendMsg::Request { display_name, note, .. } => match self.friend_store.kind(&peer) {
                Some(Kind::Blocked) => tracing::info!(%peer, "request from blocked peer dropped"),
                Some(Kind::Friend) => {
                    // Already friends (they lost us): re-confirm.
                    self.friend_store.set_name(&peer, &display_name);
                    self.announce_peer(peer);
                    self.swarm.behaviour_mut().friend.send_request(&peer, FriendMsg::Accept { schema: 1 });
                }
                Some(Kind::Out) => {
                    // Both sides asked: that's mutual consent.
                    self.friend_store.set(peer, Kind::Friend, Some(&display_name), Some(""));
                    self.announce_peer(peer);
                    self.swarm.behaviour_mut().friend.send_request(&peer, FriendMsg::Accept { schema: 1 });
                    self.emit(Event::FriendAccepted { peer, display_name });
                    self.push_presence(Some(peer));
                }
                Some(Kind::In) | None => {
                    self.friend_store.set(peer, Kind::In, Some(&display_name), Some(&note));
                    self.emit(Event::FriendRequest { peer, display_name, note });
                }
            },
            FriendMsg::Accept { .. } => {
                if self.friend_store.kind(&peer) == Some(Kind::Out) {
                    self.friend_store.add(peer);
                    self.announce_peer(peer);
                    let display_name = self.friend_store.name(&peer);
                    self.emit(Event::FriendAccepted { peer, display_name });
                    self.push_presence(Some(peer));
                }
            }
            FriendMsg::Remove { .. } => {
                // A block survives their remove.
                if matches!(self.friend_store.kind(&peer), Some(Kind::Friend | Kind::Out | Kind::In)) {
                    self.friend_store.remove(&peer);
                    self.emit(Event::FriendRemoved { peer });
                }
            }
        }
    }

    /// Surface a connected peer to the app: allocate its vip and emit
    /// PeerConnected, once. Called from every point where a peer proves
    /// app-relevant — explicit dial, friendship, session hit, inbound
    /// rexnet protocol traffic. No-op for peers we aren't connected to
    /// (or have already announced), so callers can invoke it freely.
    fn announce_peer(&mut self, peer: PeerId) {
        if !self.peer_routes.contains_key(&peer) {
            let routes: Vec<_> = self.peer_links.get(&peer).into_iter().flatten()
                .filter_map(|link| link.remote.clone()).collect();
            self.remember_peer_routes(peer, routes);
        }
        if !self.connected.contains(&peer) || !self.announced.insert(peer) {
            return;
        }
        let virtual_ip = self.vip_for(peer);
        self.emit(Event::PeerConnected { peer, virtual_ip });
        // Membership feeds the off-shard table, so our own address can move.
        // Only a collision actually shifts it, which needs a near-full subnet.
        self.announce_local_address();
    }

    /// Allocate (or return) the virtual IP for a peer.
    ///
    /// **Addresses are stable within a shard epoch, never inside one.** The
    /// guest holds an address in live sockets -- XRNM addresses its link by
    /// virtual IP -- so nothing here revises one on a whim.
    ///
    /// A shard change is the exception, because it moves the whole /24: see
    /// [`Self::readdress_all`].
    fn vip_for(&mut self, peer: PeerId) -> u32 {
        if let Some(vip) = self.vip_by_peer.get(&peer) {
            return *vip;
        }
        let vip = self.derive_vip(peer);
        self.vip_by_peer.insert(peer, vip);
        self.peer_by_vip.insert(vip, peer);
        self.plane.set_vip(peer, vip);
        vip
    }

    /// Re-derive one peer's address, reporting whether it moved.
    fn readdress_peer(&mut self, peer: PeerId) -> bool {
        let previous = self.vip_by_peer.get(&peer).copied();
        let vip = self.derive_vip(peer);
        if previous == Some(vip) {
            return false;
        }
        if let Some(old) = previous {
            self.peer_by_vip.remove(&old);
        }
        self.vip_by_peer.insert(peer, vip);
        self.peer_by_vip.insert(vip, peer);
        self.plane.set_vip(peer, vip);
        tracing::info!(
            %peer,
            from = %previous.map(format_vip).unwrap_or_else(|| "-".to_string()),
            to = %format_vip(vip),
            "peer readdressed"
        );
        // The shim mirrors the table off this event.
        if self.connected.contains(&peer) {
            self.emit(Event::PeerConnected { peer, virtual_ip: vip });
        }
        true
    }

    /// Move every address onto the current shard's /24, together.
    ///
    /// Our own address is derived fresh on every read, so it follows a shard
    /// change on its own. Peers must follow in the same step or the two sides
    /// end up on different subnets -- which silently disables the one thing a
    /// shard is for, since subnet broadcast (System Link discovery) can only
    /// reach the /24 we are actually on.
    ///
    /// Placement recomputes the whole table from one membership snapshot
    /// rather than peer by peer: `subnet::assign` resolves collisions by
    /// probing, so a partial update can hand a newcomer an address another
    /// member is still holding.
    ///
    /// Safe to do here because migration is suppressed while a session is
    /// active (§17.3.3), so this does not re-address anyone mid-co-op.
    fn readdress_all(&mut self) {
        // A peer in neither membership table cannot be derived -- `assign`
        // would not place it, and every such peer would collapse onto the same
        // fallback address. Drop them; a reconnect derives a fresh one.
        let stale: Vec<PeerId> = self
            .vip_by_peer
            .keys()
            .copied()
            .filter(|p| !self.announced.contains(p) && !self.shard_member_peers.contains(p))
            .collect();
        for peer in stale {
            if let Some(vip) = self.vip_by_peer.remove(&peer) {
                self.peer_by_vip.remove(&vip);
                self.plane.clear_vip(&peer);
            }
        }
        let peers: Vec<PeerId> = self.vip_by_peer.keys().copied().collect();
        for peer in peers {
            self.readdress_peer(peer);
        }
        self.announce_local_address();
    }

    /// Shard members get their derived address on the shard's /24 (§17.3.5);
    /// everyone else gets one from the reserved subnet.
    fn derive_vip(&mut self, peer: PeerId) -> u32 {
        if let Some(current) = self.current_shard.clone() {
            if self.shard_member_peers.contains(&peer) {
                // Our own peer id has to be in the table too, or a remote
                // could be handed the host octet we occupy.
                let mut members: Vec<PeerId> =
                    self.shard_member_peers.iter().copied().collect();
                members.push(*self.swarm.local_peer_id());
                if let Some(host) = subnet::assign(&members).get(&peer) {
                    let vip = subnet::address(VIP_NETWORK_BASE, &current.shard_id, *host);
                    tracing::debug!(
                        %peer, addr = %format_vip(vip), "shard address assigned"
                    );
                    return vip;
                }
                // Subnet full -- fall through rather than leave them
                // unaddressable and therefore unreachable by the game.
                tracing::warn!(%peer, "shard subnet full; using reserved range");
            }
        }
        self.reserved_vip_for(&peer)
    }

    /// Sequential address in 10.77.255.0/24 for a peer outside our shard.
    /// Off-shard peers, plus ourselves. Both nodes derive the same table from
    /// the same membership, which a local counter cannot do.
    fn reserved_members(&self) -> Vec<PeerId> {
        let mut members: Vec<PeerId> = self
            .announced
            .iter()
            .copied()
            .filter(|p| !self.shard_member_peers.contains(p))
            .collect();
        members.push(*self.swarm.local_peer_id());
        members
    }

    /// Address in the reserved /24, derived rather than allocated: a counter
    /// gives each node its own private numbering, so two peers never agree on
    /// who is who — fine on one machine, useless across two.
    fn reserved_vip_for(&self, peer: &PeerId) -> u32 {
        let base = VIP_NETWORK_BASE | (u32::from(subnet::RESERVED_SUBNET) << 8);
        match subnet::assign(&self.reserved_members()).get(peer) {
            Some(host) => base | u32::from(*host),
            None => {
                tracing::error!(%peer, "reserved virtual-IP subnet exhausted");
                base | u32::from(subnet::HOST_MIN)
            }
        }
    }

    /// Send anything that was waiting on this peer's punch, in order, over
    /// whichever path is now available.
    fn flush_pending_datagrams(&mut self, peer: PeerId, addr: Option<SocketAddr>) {
        let Some(queue) = self.pending_datagrams.remove(&peer) else {
            return;
        };
        let mut sent = 0usize;
        let mut stale = 0usize;
        for pending in queue {
            // A handshake that arrives after the title has given up confuses
            // it more than silence would.
            if pending.queued_at.elapsed() > pending.max_age {
                stale += 1;
                continue;
            }
            match addr {
                Some(endpoint) => {
                    if let Some(frame) =
                        self.seal_datagram(peer, pending.src_port, pending.dst_port, &pending.data)
                    {
                        self.plane.send_frame(peer, endpoint, &frame, pending.src_port, pending.dst_port);
                    }
                }
                None => self.send_tunneled(
                    peer,
                    pending.src_port,
                    pending.dst_port,
                    pending.data,
                ),
            }
            sent += 1;
        }
        if sent > 0 || stale > 0 {
            let path = if addr.is_some() { "punched" } else { "tunnel" };
            self.diagnostics.record("punch_queue_flushed",serde_json::json!({"peer":peer.to_base58(),
                "attempted":sent,"expired":stale,"path":path}));
            tracing::info!(%peer, sent, stale, path, "flushed datagrams held for punch");
        }
    }

    /// Deliver frames that arrived from `addr` before it was mapped to a peer.
    fn deliver_orphan_frames(&mut self, peer: PeerId, addr: SocketAddr) {
        let Some(queue) = self.orphan_frames.remove(&addr) else {
            return;
        };
        let virtual_ip = self.vip_for(peer);
        let mut delivered = 0usize;
        for (received_at, frame) in queue {
            if received_at.elapsed() > PUNCH_QUEUE_MAX_AGE
                || frame.len() < 1 + crypto::CRYPTO_OVERHEAD
            {
                continue;
            }
            // Buffered before the address mapped to a peer, so they could not
            // be decrypted then: keys are per peer and records carry nothing
            // identifying.
            let Some((src_port, dst_port, data)) = self.open_datagram(peer, &frame) else {
                continue;
            };
            self.emit(Event::Datagram { virtual_ip, src_port, dst_port, data });
            delivered += 1;
        }
        if delivered > 0 {
            tracing::info!(%peer, %addr, delivered, "delivered frames held before punch");
        }
    }

    /// Drop orphan frames whose sender never became a known peer.
    fn prune_orphan_frames(&mut self) {
        self.orphan_frames.retain(|_, queue| {
            queue.retain(|(at, _)| at.elapsed() <= PUNCH_QUEUE_MAX_AGE);
            !queue.is_empty()
        });
    }

    /// A punched endpoint is only usable once it is also keyed; before that
    /// nothing can be sealed, so treating it as ready strands the peer.
    fn game_plane_ready(&self, peer: &PeerId) -> bool {
        self.plane.ready(peer)
    }

    /// The punched endpoint, only once it is also keyed.
    fn ready_endpoint(&self, peer: &PeerId) -> Option<SocketAddr> {
        self.plane.endpoint(peer).filter(|_| self.plane.has_keys(peer))
    }

    /// Reused across punch attempts so both sides agree (§6.1).
    fn local_game_pubkey(&mut self, peer: PeerId) -> [u8; 32] {
        self.key_agreements.entry(peer).or_default().public_key()
    }

    /// An established session is left alone when the same key arrives again
    /// (simultaneous punches deliver it twice): re-deriving would rewind the
    /// nonce counter and replay window. A *different* key can only come from
    /// a peer that restarted, so the session is rebuilt around it -- endpoint
    /// included, since the old one belongs to the old process.
    fn establish_game_keys(&mut self, peer: PeerId, peer_pub: [u8; 32]) {
        if self.plane.has_keys(&peer) {
            if self.peer_game_pubs.get(&peer) == Some(&peer_pub) {
                return;
            }
            tracing::info!(%peer, "peer presented a new game-plane key; rebuilding its session");
            self.plane.remove_keys(&peer);
            self.plane.unmap_endpoint(&peer);
        }
        let keys = self.key_agreements.entry(peer).or_default().derive(&peer_pub);
        self.plane.install_keys(peer, keys);
        self.peer_game_pubs.insert(peer, peer_pub);
        tracing::info!(%peer, "game-plane keys established");
        if let Some(addr) = self.plane.endpoint(&peer) {
            self.complete_game_plane(peer, addr);
        }
    }

    /// Engine-side sealing for held and broadcast traffic; live traffic goes
    /// through `GamePlane::try_send` without touching this loop.
    fn seal_datagram(
        &mut self,
        peer: PeerId,
        src_port: u16,
        dst_port: u16,
        data: &[u8],
    ) -> Option<Vec<u8>> {
        self.plane.seal(peer, src_port, dst_port, data)
    }

    /// Engine-side decryption for frames held before their endpoint mapped.
    fn open_datagram(&mut self, peer: PeerId, frame: &[u8]) -> Option<(u16, u16, Vec<u8>)> {
        self.plane.open(peer, frame)
    }

    /// Record a round trip measured from a probe ack.
    ///
    /// Smoothed rather than replaced: a single sample on a busy link is noise,
    /// and a title showing ping wants a stable number.
    fn note_rtt(&mut self, addr: SocketAddr, sent_micros: u64) {
        let Some(peer) = self.plane.peer_at(&addr) else {
            return;
        };
        let now = now_micros();
        if now < sent_micros {
            return; // stamp from a previous engine, or a forged ack
        }
        let sample = ((now - sent_micros) / 1000).min(u32::MAX as u64) as u32;
        let smoothed = match self.peer_rtt_ms.get(&peer) {
            // 1/4 weight on the newest sample, as TCP's SRTT does.
            Some(prev) => (prev * 3 + sample) / 4,
            None => sample,
        };
        let changed = self
            .peer_rtt_ms
            .insert(peer, smoothed)
            .map_or(true, |prev| prev.abs_diff(smoothed) >= RTT_REPORT_DELTA_MS);
        if changed {
            let virtual_ip = self.vip_for(peer);
            self.emit(Event::PeerRtt { virtual_ip, rtt_ms: smoothed });
        }
    }

    /// Carry one datagram over the control connection (§14 degraded path).
    /// This peer's tunnel writer, opening the stream if there is none.
    /// Opening is asynchronous, so the channel exists before the stream does:
    /// frames sent in that window queue rather than being lost, and the writer
    /// task drains them the moment the stream comes up.
    fn tunnel_writer(&mut self, peer: PeerId) -> &mut mpsc::Sender<(Instant, Vec<u8>)> {
        let next = &mut self.next_tunnel_writer;
        &mut self.tunnel_writers.entry(peer).or_insert_with(|| {
            *next += 1;
            let (tx, rx) = mpsc::channel::<(Instant, Vec<u8>)>(TUNNEL_WRITE_QUEUE);
            spawn_tunnel_writer(self.tunnel_control.clone(), peer, *next, rx, self.tunnel_tx.clone(), self.diagnostics.clone());
            (*next, tx)
        }).1
    }

    fn send_tunneled(&mut self, peer: PeerId, src_port: u16, dst_port: u16, data: Vec<u8>) {
        let Some(frame) = tunnel::encode(src_port, dst_port, &data) else {
            self.diagnostics.record("tunnel_drop",serde_json::json!({"peer":peer.to_base58(),
                "src_port":src_port,"dst_port":dst_port,"bytes":data.len(),"reason":"too_large"}));
            tracing::warn!(%peer, len = data.len(), "datagram too large for the tunnel, dropped");
            return;
        };
        let len = frame.len() as u64;
        let age = if self.recovering.contains_key(&peer) { PEER_RECOVERY_RETRY } else { PUNCH_QUEUE_MAX_AGE };
        let result = self.tunnel_writer(peer).try_send((Instant::now() + age, frame));
        self.diagnostics.record(if result.is_ok() {"tunnel_queued"} else {"tunnel_drop"},
            serde_json::json!({"peer":peer.to_base58(),"src_port":src_port,"dst_port":dst_port,
                "bytes":data.len(),"reason":result.as_ref().err().map(|e|match e {
                    mpsc::error::TrySendError::Full(_) => "queue_full",
                    mpsc::error::TrySendError::Closed(_) => "writer_closed"})}));
        if result.is_err() {
            // Full (congested) or closed (the task is on its way out and will
            // report WriterGone). Either way this datagram is gone -- which is
            // what "UDP" means to the guest.
            *self.tunnel_drops.entry(peer).or_insert(0) += 1;
        } else {
            self.tunnel_packets.entry(peer).or_default().0 += 1;
            *self.tunnel_bytes.entry(peer).or_default() += len;
        }
    }

    fn handle_tunnel_event(&mut self, event: TunnelEvent) {
        match event {
            TunnelEvent::FrameExpired { peer, writer_id } => {
                if current_tunnel_writer(&self.tunnel_writers, &peer, writer_id) {
                    *self.tunnel_drops.entry(peer).or_default() += 1;
                    self.diagnostics.record("tunnel_drop", serde_json::json!({"peer":peer.to_base58(),"reason":"expired","writer_id":writer_id}));
                }
            }
            TunnelEvent::WriterOpened { peer, writer_id } => {
                if current_tunnel_writer(&self.tunnel_writers, &peer, writer_id) {
                    self.game_path_recovered(peer, "tunnel");
                }
            }
            TunnelEvent::ReaderClosed {peer,error} => {
                self.diagnostics.incident("tunnel_reader_closed",serde_json::json!({"peer":peer.to_base58(),"error":error}));
            }
            TunnelEvent::Opened { peer } => {
                if !self.connected.contains(&peer) { return; }
                self.mirror_peer_tunnel(peer);
                self.game_path_recovered(peer, "tunnel");
            }
            TunnelEvent::Frame { peer, src_port, dst_port, data } => {
                if !self.connected.contains(&peer) { return; }
                self.tunnel_packets.entry(peer).or_default().1 += 1;
                *self.tunnel_bytes.entry(peer).or_default() += data.len() as u64;
                self.mirror_peer_tunnel(peer);
                self.game_path_recovered(peer, "tunnel");
                self.announce_peer(peer);
                let virtual_ip = self.vip_for(peer);
                // Surfaced exactly as a punched datagram would be: the guest
                // must not be able to tell which path carried it.
                self.emit(Event::Datagram { virtual_ip, src_port, dst_port, data });
            }
            TunnelEvent::WriterGone { peer, writer_id, error } => {
                if !current_tunnel_writer(&self.tunnel_writers, &peer, writer_id) {
                    tracing::debug!(%peer, writer_id, "ignoring retired tunnel writer closure");
                    return;
                }
                if error.is_some() {
                    self.diagnostics.incident("tunnel_writer_failed", serde_json::json!({"peer":peer.to_base58(),
                        "writer_id":writer_id,"error":error}));
                }
                // Drop the writer rather than the tunneled marker: the peer is
                // still unreachable by punch, so the next datagram should open
                // a fresh stream instead of falling back to nothing.
                self.tunnel_writers.remove(&peer);
                let drops = self.tunnel_drops.remove(&peer).unwrap_or(0);
                match error {
                    Some(err) => tracing::warn!(%peer, drops, %err, "tunnel writer failed"),
                    None => tracing::info!(%peer, drops, "tunnel stream closed"),
                }
            }
        }
    }

    /// WriterGone includes its generation, so a retired task cannot tear
    /// down a replacement created during reconnect or path switching.
    fn close_tunnel(&mut self, peer: &PeerId) {
        // Dropping the sender ends the writer task, which closes the stream.
        if self.tunnel_writers.remove(peer).is_some() {
            let drops = self.tunnel_drops.remove(peer).unwrap_or(0);
            if drops > 0 {
                tracing::warn!(%peer, drops, "tunnel dropped datagrams while degraded");
            }
        }
        self.tunnel_drops.remove(peer);
    }

    /// A punch has run out of time: fall back to the tunnel so the player can
    /// still connect, and tell the shim the link is degraded.
    fn tunnel_fallback(&mut self, peer: PeerId) {
        if !self.tunneled_peers.insert(peer) {
            return;
        }
        tracing::warn!(
            %peer,
            "punch failed; game traffic falling back to the control tunnel (degraded)"
        );
        self.emit(Event::Degraded { peer });
        self.flush_pending_datagrams(peer, None);
        // Open the tunnel now rather than at our first datagram: its opening
        // is what tells the peer to answer through it, and a host that only
        // speaks when spoken to would otherwise never hear a joiner.
        self.tunnel_writer(peer);
    }

    /// A peer tunnels to us only because its punch failed, which means our
    /// direct path is not one it keeps open, even when our own punch
    /// succeeded (simultaneous punches, or a NAT or firewall that passed one
    /// direction). What we send down it is lost: MC3's host answered searches
    /// the joiner never heard. Answer through the tunnel, as the peer does.
    fn mirror_peer_tunnel(&mut self, peer: PeerId) {
        if self.tunneled_peers.contains(&peer) {
            return;
        }
        if self.plane.unmap_endpoint(&peer).is_some() {
            tracing::info!(%peer, "peer tunnels to us; dropping our punched path to match");
        }
        self.pending_punch_nonce.retain(|_, p| *p != peer);
        self.punch_deadline.remove(&peer);
        self.tunnel_fallback(peer);
    }

    /// Give up on punches that have exceeded PUNCH_TIMEOUT.
    fn check_punch_deadlines(&mut self) {
        self.prune_orphan_frames();
        let expired: Vec<PeerId> = self
            .punch_deadline
            .iter()
            .filter(|(peer, deadline)| {
                Instant::now() >= **deadline && !self.game_plane_ready(peer)
            })
            .map(|(peer, _)| *peer)
            .collect();
        for peer in expired {
            self.punch_deadline.remove(&peer);
            self.tunnel_fallback(peer);
        }
    }

    /// One line saying what the shard is doing, so its state can be read at a
    /// glance instead of reconstructed from scattered events.
    fn log_shard_status(&self) {
        if !self.shard_enabled {
            return;
        }
        let Some(current) = &self.current_shard else {
            tracing::info!(
                candidates = self.shard_candidates.len(),
                swept = self.shard_swept_at.is_some(),
                "shard: none yet"
            );
            return;
        };
        let topic = gossipsub::IdentTopic::new(crate::protocol::shard_topic(&current.shard_id));
        let hash = topic.hash();
        let subscribed = self
            .swarm
            .behaviour()
            .gossipsub
            .all_peers()
            .filter(|(_, topics)| topics.iter().any(|t| **t == hash))
            .count();
        tracing::info!(
            shard = %hex16(&current.shard_id),
            addr = %format_vip(self.local_vip()),
            members_heard = self.shard_member_peers.len(),
            topic_peers = subscribed,
            reachable = self
                .shard_member_peers
                .iter()
                .filter(|p| self.connected.contains(p))
                .count(),
            tunneled = self.tunneled_peers.len(),
            "shard status"
        );
        for peer in &self.shard_member_peers {
            let health = self.plane.health(peer);
            let (tunnel_queued, tunnel_received) = self.tunnel_packets.get(peer).copied().unwrap_or_default();
            tracing::info!(%peer, control_connected = self.connected.contains(peer),
                direct_ready = self.game_plane_ready(peer),
                tunnel = self.tunneled_peers.contains(peer),
                writer = self.tunnel_writers.contains_key(peer),
                punch_pending = self.punch_deadline.contains_key(peer),
                udp_sealed = health.sealed, udp_received = health.received,
                udp_rejected = health.rejected, udp_send_errors = health.send_errors,
                udp_silence_ms = ?health.last_rx.map(|at| at.elapsed().as_millis()),
                game_rtt_ms = ?self.peer_rtt_ms.get(peer),
                tunnel_queued, tunnel_received,
                tunnel_drops = self.tunnel_drops.get(peer).copied().unwrap_or(0),
                "peer link health");
        }
    }

    fn diagnostic_health(&self, engine_gap_ms: u128) {
        if !self.diagnostics.enabled() { return; }
        let queries: Vec<_> = self.swarm.behaviour().kad.iter_queries().map(|query| {
            let id = query.id();
            let kind = match query.info() {
                kad::QueryInfo::Bootstrap { .. } => "bootstrap",
                kad::QueryInfo::GetClosestPeers { .. } => "find_peer",
                kad::QueryInfo::GetProviders { .. } => "find_providers",
                kad::QueryInfo::AddProvider { .. } => "publish_provider",
                kad::QueryInfo::PutRecord { .. } => "put_record",
                kad::QueryInfo::GetRecord { .. } => "get_record",
            };
            let source = if self.peer_lookups.contains_key(&id) { "friend_or_notice" }
                else if self.pending_session_search.as_ref().is_some_and(|(q,_)|*q==id) { "session_search" }
                else if self.pending_shard_search.as_ref().is_some_and(|(q,_)|*q==id) { "shard_search" }
                else if self.pending_relay_search==Some(id) { "relay_search" }
                else if self.seed_searches.contains_key(&id) { "notice_seed" }
                else if self.roster_searches.contains_key(&id) { "notice_roster" }
                else { kind };
            let stats = query.stats();
            serde_json::json!({"id":format!("{id:?}"),"kind":kind,"source":source,
                "requests":stats.num_requests(),"successes":stats.num_successes(),
                "failures":stats.num_failures(),"pending":stats.num_pending(),
                "duration_ms":stats.duration().map(|d|d.as_millis())})
        }).collect();
        self.diagnostics.record("engine_health", serde_json::json!({"engine_gap_ms":engine_gap_ms,
            "connected":self.connected.len(),"identified_rexnet_connected":self.connected.intersection(&self.rexnet_peers).count(),
            "pending_outgoing":self.swarm.network_info().connection_counters().num_pending_outgoing(),
            "dht_queries":queries,"shard_members":self.shard_member_peers.len(),"recovering":self.recovering.len()}));
        for peer in self.shard_member_peers.union(&self.announced) {
            let health = self.plane.health(peer);
            let (queued, received) = self.tunnel_packets.get(peer).copied().unwrap_or_default();
            let links: Vec<_> = self.peer_links.get(peer).into_iter().flatten().map(|link|
                serde_json::json!({"id":link.id.to_string(),"relayed":link.relayed,"quic":link.quic,"inbound":link.inbound,
                    "remote":link.remote.as_ref().map(|a|a.to_string()),
                    "renew_in_ms":link.renew_at.map(|at|at.saturating_duration_since(Instant::now()).as_millis()),
                    "renew_bytes":link.renew_bytes})).collect();
            self.diagnostics.record("peer_health", serde_json::json!({"peer":peer.to_base58(),
                "virtual_ip":self.vip_by_peer.get(peer),"control_connected":self.connected.contains(peer),
                "recovering_ms":self.recovering.get(peer).map(|r|r.started.elapsed().as_millis()),
                "cached_routes":self.peer_routes.get(peer).map_or(0, |r|r.addresses.len()),
                "direct_ready":self.game_plane_ready(peer),"endpoint":self.plane.endpoint(peer).map(|p|p.to_string()),
                "tunnel":self.tunneled_peers.contains(peer),"writer":self.tunnel_writers.contains_key(peer),
                "punch_pending":self.punch_deadline.contains_key(peer),"udp_sealed":health.sealed,
                "udp_received":health.received,"udp_rejected":health.rejected,"udp_send_errors":health.send_errors,
                "udp_silence_ms":health.last_rx.map(|at|at.elapsed().as_millis()),
                "rtt_ms":self.peer_rtt_ms.get(peer),"tunnel_queued":queued,"tunnel_received":received,
                "tunnel_drops":self.tunnel_drops.get(peer).copied().unwrap_or(0),"links":links}));
        }
    }

    /// Turn a pump-task report into guest-facing state and events.
    fn handle_stream_event(&mut self, event: StreamEvent) {
        match event {
            StreamEvent::Opened { id, peer, local_port, remote_port, outbound, writer } => {
                let virtual_ip = self.vip_for(peer);
                self.streams.insert(StreamInfo { peer, local_port, remote_port, outbound });
                self.stream_writers.insert(id, writer);
                // A peer we are exchanging TCP with is app-relevant even if
                // nothing else made it so.
                self.announce_peer(peer);
                tracing::info!(
                    stream_id = id, %peer, local_port, remote_port, outbound,
                    "guest TCP connection open"
                );
                self.emit(Event::StreamOpened {
                    stream_id: id,
                    virtual_ip,
                    local_port,
                    remote_port,
                    outbound,
                });
            }
            StreamEvent::Data { id, data } => {
                self.emit(Event::StreamData { stream_id: id, data });
            }
            StreamEvent::Closed { id } => {
                self.streams.remove(id);
                self.stream_writers.remove(&id);
                tracing::info!(stream_id = id, "guest TCP connection closed");
                self.emit(Event::StreamClosed { stream_id: id });
            }
            StreamEvent::ConnectFailed { peer, dst_port, error } => {
                let virtual_ip = self.vip_for(peer);
                tracing::warn!(%peer, dst_port, %error, "guest TCP connect failed");
                self.emit(Event::StreamConnectFailed {
                    virtual_ip,
                    dst_port,
                    message: error,
                });
            }
        }
    }

    /// Kick a NAT punch toward `peer`.
    fn begin_punch(&mut self, peer: PeerId) {
        if self.recovering.contains_key(&peer) && !self.connected.contains(&peer) {
            return; // recovery owns the dial policy; request-response would redial QUIC
        }
        if self.force_tunnel {
            tracing::debug!(%peer, "punch skipped (force_tunnel)");
            self.tunnel_fallback(peer);
            return;
        }
        self.punch_deadline.entry(peer).or_insert_with(|| Instant::now() + PUNCH_TIMEOUT);
        let nonce: [u8; 16] = rand::random();
        let candidates = self.local_candidates();
        self.pending_punch_nonce.insert(nonce, peer);
        self.diagnostics.record("punch_offer_sent",serde_json::json!({"peer":peer.to_base58(),
            "correlation":nonce.iter().map(|b|format!("{b:02x}")).collect::<String>(),"candidates":candidates}));
        let eph_pub = self.local_game_pubkey(peer);
        tracing::info!(
            %peer,
            candidates = ?candidates,
            local_addr = %format_vip(self.local_vip()),
            "punch offer sent"
        );
        let id = self
            .swarm
            .behaviour_mut()
            .punch
            .send_request(&peer, PunchOffer { nonce, candidates, eph_pub });
        self.pending_punch_offers.insert(id, peer);
    }

    /// Is this address a broadcast for our shard?
    ///
    /// Accepts both the limited broadcast a System Link title typically uses
    /// (255.255.255.255) and our own subnet's directed broadcast. Titles use
    /// either, and on a real LAN both would reach the same machines.
    fn is_broadcast_vip(&self, vip: u32) -> bool {
        if vip == u32::MAX {
            return true;
        }
        match &self.current_shard {
            Some(current) => {
                vip == subnet::broadcast_address(VIP_NETWORK_BASE, &current.shard_id)
            }
            None => false,
        }
    }

    /// Deliver a datagram to every shard member.
    ///
    /// Members without a punched game-plane endpoint cannot be reached, so a
    /// punch is started for them: discovery broadcasts repeat, and the next
    /// one gets through. Punching lazily like this means only titles that
    /// actually broadcast pay for it, rather than every shard punching a full
    /// mesh on the chance someone might.
    fn broadcast_datagram(&mut self, src_port: u16, dst_port: u16, data: &[u8]) {
        // Sealed once per recipient rather than built once and reused: keys
        // are per peer (§6), so there is no such thing as a frame every member
        // can read. At a full 255-member shard that is 255 encryptions per
        // broadcast — cheap per datagram, but it scales with the shard and is
        // one of the things §17.6 flags as unmeasured at that size.
        let members: Vec<PeerId> = self.shard_member_peers.iter().copied().collect();
        let mut sent = 0usize;
        let mut punching = 0usize;
        for peer in members {
            if self.recovering.contains_key(&peer) && !self.connected.contains(&peer) { continue; }
            match self.plane.endpoint(&peer) {
                Some(endpoint) if self.plane.has_keys(&peer) => {
                    if let Some(frame) = self.seal_datagram(peer, src_port, dst_port, data) {
                        self.plane.send_frame(peer, endpoint, &frame, src_port, dst_port);
                        sent += 1;
                    }
                }
                // Punched but not yet keyed. Skipped quietly: the beat repeats.
                Some(_) => {}
                // Its punch already failed, so unicast reaches it through the
                // tunnel; a discovery shout must too, or a title never lists a
                // host it can only reach that way (MC3's Join Game list).
                None if self.tunneled_peers.contains(&peer) => {
                    self.send_tunneled(peer, src_port, dst_port, data.to_vec());
                    sent += 1;
                }
                None => {
                    if self.pending_punch_offers.values().all(|p| *p != peer) {
                        self.begin_punch(peer);
                    }
                    punching += 1;
                }
            }
        }
        tracing::debug!(dst_port, sent, punching, "shard broadcast");
    }

    /// Tell the shim our current address, so the XNADDR it reports to the
    /// game matches the subnet we are actually on.
    /// Record a confirmed public endpoint (from UPnP or AutoNAT) and, the first
    /// time we see it, surface a bare `host:port` for the player to hand to a
    /// peer for a manual direct connect (§9 direct path).
    fn note_external_addr(&mut self, address: Multiaddr) {
        // A circuit address carries the relay's IP, not ours. Taken as ours it
        // made punch probes target relay servers and could advertise a NAT'd
        // node as a mesh relay.
        if address.iter().any(|proto| proto == Protocol::P2pCircuit) {
            return;
        }
        for proto in address.iter() {
            match proto {
                Protocol::Ip4(ip) => self.external_ips.push(ip.into()),
                Protocol::Ip6(ip) => self.external_ips.push(ip.into()),
                _ => {}
            }
        }
        self.external_ips.sort();
        self.external_ips.dedup();

        if let Some(endpoint) = dialable_endpoint(&address) {
            if self.announced_external.insert(endpoint.clone()) {
                self.emit(Event::ExternalAddress { addr: endpoint });
            }
        }

        // Now that we are confirmed reachable on a routable address, advertise
        // ourselves as a relay in the DHT (§9): NAT'd peers query the same key
        // to find a relay to punch through, including ones they have never met.
        if !self.relay_provider_registered && multiaddr_is_global(&address) {
            let key = kad::RecordKey::new(&crate::protocol::RELAYS_KEY);
            match self.swarm.behaviour_mut().kad.start_providing(key) {
                Ok(_) => {
                    self.relay_provider_registered = true;
                    tracing::info!("registered as a mesh relay provider (reachable)");
                }
                Err(err) => tracing::debug!(%err, "relay provider registration failed"),
            }
        }
    }

    /// Query the DHT for `rexnet/v1/relays` providers (§9) when we still need
    /// relays. Found providers are dialled; the identify handler then reserves a
    /// circuit through any that offer the hop on a routable address. This is the
    /// active half of relay discovery -- it finds relays we have never connected
    /// to, which identify-on-existing-connections alone cannot.
    fn discover_relays(&mut self) {
        if self.auto_relays.len() >= MAX_AUTO_RELAYS || self.pending_relay_search.is_some() {
            return;
        }
        let key = kad::RecordKey::new(&crate::protocol::RELAYS_KEY);
        let id = self.swarm.behaviour_mut().kad.get_providers(key);
        self.pending_relay_search = Some(id);
    }

    /// Hold an automatic circuit reservation on a connected, publicly reachable
    /// mesh peer that offers the circuit-v2 hop (§9 torrent model). This is how
    /// a NAT'd node gets a relay from the swarm itself, with no configured
    /// server. Capped at [`MAX_AUTO_RELAYS`] so a large mesh does not become an
    /// N-squared reservation storm.
    fn maybe_auto_reserve_relay(
        &mut self,
        peer_id: PeerId,
        protocols: &[StreamProtocol],
        listen_addrs: &[Multiaddr],
    ) {
        if self.auto_relays.len() >= MAX_AUTO_RELAYS || self.auto_relays.contains(&peer_id) {
            return;
        }
        // Peer must speak the circuit-v2 hop, or it cannot relay for us.
        let offers_hop = protocols
            .iter()
            .any(|p| p.as_ref() == "/libp2p/circuit/relay/0.2.0/hop");
        if !offers_hop {
            return;
        }
        let rexnet = protocols.iter().any(|p| p.as_ref() == crate::protocol::SHARD);
        let holding_rexnet = self.auto_relay_listeners.values().any(|&(_, rexnet)| rexnet);
        if !auto_relay_eligible(
            self.bootstrap_peers.contains(&peer_id),
            self.auto_relay_backoff.get(&peer_id).copied(),
            rexnet,
            holding_rexnet,
            self.no_rexnet_relay_since,
            Instant::now(),
        ) {
            return;
        }
        // Reserve only through a globally routable address: a LAN-only relay is
        // useless to a NAT'd peer elsewhere that must reach us through it.
        let Some(base) = reservable_addr(listen_addrs) else {
            return;
        };
        // Circuit = <relay transport>/p2p/<relay>/p2p-circuit. Drop any trailing
        // /p2p the relay already advertised so it is not duplicated.
        let mut circuit = base.clone();
        if matches!(circuit.iter().last(), Some(Protocol::P2p(_))) {
            circuit.pop();
        }
        let circuit = circuit.with(Protocol::P2p(peer_id)).with(Protocol::P2pCircuit);
        match self.swarm.listen_on(circuit.clone()) {
            Ok(listener) => {
                self.auto_relays.insert(peer_id);
                self.auto_relay_listeners.insert(listener, (peer_id, rexnet));
                tracing::info!(%peer_id, %circuit, rexnet,
                    "mesh relay: auto-reserving a circuit through a reachable peer");
            }
            Err(err) => {
                tracing::debug!(%peer_id, %err, "auto relay reservation failed to start");
            }
        }
    }

    /// An auto relay's circuit listener closed. libp2p-relay 0.21 has no
    /// failure event: a refused or failed reservation (a renewal included)
    /// closes the listener with an error, and the relay's connection closing
    /// closes it without one. Either way the slot holds nothing now. A failure
    /// also backs the relay off, so the next identify does not pick it again.
    fn auto_relay_listener_closed(&mut self, listener: libp2p::core::transport::ListenerId, failed: bool) {
        let Some((peer, rexnet)) = self.auto_relay_listeners.remove(&listener) else { return };
        let now = Instant::now();
        if !self.auto_relay_listeners.values().any(|&(other, _)| other == peer) {
            self.auto_relays.remove(&peer);
        }
        if rexnet {
            self.no_rexnet_relay_since = now;
        }
        if failed {
            self.auto_relay_backoff.retain(|_, until| now < *until);
            self.auto_relay_backoff.insert(peer, now + AUTO_RELAY_BACKOFF);
            tracing::info!(%peer, "mesh relay: reservation refused or failed; slot freed, relay backed off");
        }
    }

    fn announce_local_address(&mut self) {
        let vip = self.local_vip();
        if self.announced_vip == Some(vip) {
            return;
        }
        self.announced_vip = Some(vip);
        tracing::info!(addr = %format_vip(vip), "local address");
        self.emit(Event::LocalAddress { virtual_ip: vip });
    }

    /// Our own address: our host on the shard's /24 when we are in one, else
    /// the reserved-range identity the shim uses before any shard exists.
    fn local_vip(&self) -> u32 {
        if let Some(current) = &self.current_shard {
            let mut members: Vec<PeerId> = self.shard_member_peers.iter().copied().collect();
            let local = *self.swarm.local_peer_id();
            members.push(local);
            if let Some(host) = subnet::assign(&members).get(&local) {
                return subnet::address(VIP_NETWORK_BASE, &current.shard_id, *host);
            }
        }
        self.reserved_vip_for(self.swarm.local_peer_id())
    }

    /// Candidate endpoints for the punch: every local interface we listen on
    /// plus AutoNAT-confirmed external addresses, all with the game port.
    /// (External candidates assume the NAT maps the game port unchanged —
    /// true for full-cone/port-preserving NATs; the punch tries all pairs.)
    fn local_candidates(&self) -> Vec<SocketAddr> {
        let game_port = match self.game_socket.local_addr() {
            Ok(addr) => addr.port(),
            Err(_) => return Vec::new(),
        };
        let mut ips: Vec<IpAddr> = Vec::new();
        for addr in &self.listen_addrs {
            for proto in addr.iter() {
                match proto {
                    Protocol::Ip4(ip) if !ip.is_unspecified() => ips.push(ip.into()),
                    Protocol::Ip6(ip) if !ip.is_unspecified() => ips.push(ip.into()),
                    _ => {}
                }
            }
        }
        ips.extend(self.external_ips.iter().copied());
        ips.sort();
        ips.dedup();
        ips.into_iter().map(|ip| SocketAddr::new(ip, game_port)).collect()
    }

    fn spawn_probes(&self, nonce: [u8; 16], candidates: Vec<SocketAddr>) {
        let socket = self.game_socket.clone();
        let diagnostics = self.diagnostics.clone();
        tokio::spawn(async move {
            for _ in 0..PROBE_ROUNDS {
                // Stamped per round: the peer echoes it back untouched, so the
                // round trip is measured entirely against our own clock and
                // needs no agreement about time.
                let mut probe = Vec::with_capacity(PROBE_FRAME_LEN);
                probe.push(FRAME_PROBE);
                probe.extend_from_slice(&nonce);
                probe.extend_from_slice(&now_micros().to_be_bytes());
                for addr in &candidates {
                    if let Ok(bytes) = socket.send_to(&probe, addr).await {
                        diagnostics.record("udp_probe_sent",serde_json::json!({"wire_bytes":bytes,"endpoint":addr.to_string(),"reason":"punch"}));
                    }
                }
                tokio::time::sleep(PROBE_INTERVAL).await;
            }
        });
    }

    /// A probe (or probe-ack) with a known nonce arrived from `addr`:
    /// that address is the peer's working game endpoint.
    fn lock_game_endpoint(&mut self, nonce: [u8; 16], addr: SocketAddr) {
        let Some(peer) = self.pending_punch_nonce.get(&nonce).copied() else {
            tracing::trace!(%addr, "probe with unknown nonce ignored");
            return;
        };
        if self.plane.endpoint(&peer).is_some() {
            return; // first authenticated pair wins (§8.4)
        }
        self.diagnostics.record("punch_probe_received",serde_json::json!({"peer":peer.to_base58(),
            "correlation":nonce.iter().map(|b|format!("{b:02x}")).collect::<String>(),"endpoint":addr.to_string()}));
        tracing::info!(
            %peer, %addr,
            peer_addr = %format_vip(self.vip_by_peer.get(&peer).copied().unwrap_or(0)),
            "game endpoint punched"
        );
        self.plane.map_endpoint(peer, addr);
        self.vip_for(peer);
        // The probe travels direct while the punch answer comes back over the
        // control connection, so the endpoint can map before keys exist.
        // Whichever lands second finishes the job; until then the deadline
        // stays armed so the watchdog can still fall back to the tunnel.
        if self.plane.has_keys(&peer) {
            self.complete_game_plane(peer, addr);
        } else {
            tracing::warn!(%peer, %addr, "endpoint punched but no keys yet; holding traffic");
        }
    }

    /// Endpoint and keys are both in place: the plane can carry traffic.
    fn complete_game_plane(&mut self, peer: PeerId, addr: SocketAddr) {
        self.game_path_recovered(peer, "udp");
        self.punch_deadline.remove(&peer);
        if self.tunneled_peers.remove(&peer) {
            tracing::info!(%peer, "punch succeeded; leaving the degraded tunnel");
            self.close_tunnel(&peer);
        }
        self.flush_pending_datagrams(peer, Some(addr));
        self.deliver_orphan_frames(peer, addr);
        self.emit(Event::PunchResult { peer, ok: true });
        if let Some(reply) = self.punch_replies.remove(&peer) {
            let _ = reply.send(Ok(addr));
        }
    }

    fn handle_game_frame(&mut self, addr: SocketAddr, frame: Vec<u8>) {
        match frame.first().copied() {
            Some(FRAME_DATA) => {
                // [0x01][counter u64be][ChaCha20-Poly1305 ciphertext+tag]
                if frame.len() < 1 + crypto::CRYPTO_OVERHEAD {
                    return;
                }
                // Unmapped, or mapped but not yet keyed because the probe beat
                // the punch answer. Either way hold rather than drop: this
                // window is exactly when a join's opening handshake arrives.
                let mapped = self.plane.peer_at(&addr);
                let Some(peer) = mapped.filter(|p| self.plane.has_keys(p)) else {
                    // The two sides complete their punches independently, and
                    // the peer flushes whatever it queued the instant *its*
                    // side completes -- which can be milliseconds before ours.
                    // Dropping here discards the opening handshake of a join
                    // and the title then waits out its own timeout. Hold the
                    // frames until the endpoint is mapped instead; this is the
                    // receive-side mirror of the send-side punch queue.
                    let queue = self.orphan_frames.entry(addr).or_default();
                    if queue.len() >= PUNCH_QUEUE_MAX {
                        queue.remove(0);
                    }
                    queue.push((Instant::now(), frame));
                    tracing::debug!(%addr, queued = queue.len(), "datagram held: endpoint not mapped yet");
                    return;
                };
                let Some((src_port, dst_port, data)) = self.open_datagram(peer, &frame) else {
                    return;
                };
                let virtual_ip = self.vip_for(peer);
                self.emit(Event::Datagram { virtual_ip, src_port, dst_port, data });
            }
            Some(FRAME_PROBE) | Some(FRAME_PROBE_ACK) if frame.len() == PROBE_FRAME_LEN => {
                let mut nonce = [0u8; 16];
                nonce.copy_from_slice(&frame[1..17]);
                let is_probe = frame[0] == FRAME_PROBE;
                self.lock_game_endpoint(nonce, addr);
                if is_probe {
                    // Echo the stamp back verbatim; its meaning is the
                    // sender's alone.
                    let mut ack = Vec::with_capacity(PROBE_FRAME_LEN);
                    ack.push(FRAME_PROBE_ACK);
                    ack.extend_from_slice(&frame[1..PROBE_FRAME_LEN]);
                    if let Ok(bytes) = self.game_socket.try_send_to(&ack, addr) {
                        self.diagnostics.record("udp_probe_sent",serde_json::json!({"wire_bytes":bytes,"endpoint":addr.to_string(),"reason":"ack"}));
                    }
                } else {
                    let mut stamp = [0u8; 8];
                    stamp.copy_from_slice(&frame[17..PROBE_FRAME_LEN]);
                    self.note_rtt(addr, u64::from_be_bytes(stamp));
                }
            }
            _ => tracing::trace!(%addr, len = frame.len(), "bad game frame dropped"),
        }
    }

    /// Returns false on shutdown.
    fn handle_command(&mut self, cmd: Command) -> bool {
        match cmd {
            Command::ConnectPeer { peer } => {
                // Addresses come from kad/mdns/identify via the behaviours'
                // address book; kick a DHT lookup in parallel for peers we
                // have never seen.
                self.swarm.behaviour_mut().kad.get_closest_peers(peer);
                self.wanted_peers.insert(peer);
                // Already connected (e.g. via a session descriptor query):
                // the dial below no-ops, so surface the peer here.
                self.announce_peer(peer);
                if let Err(err) = self.swarm.dial(peer) {
                    self.emit(Event::Error { message: format!("dial {peer}: {err}") });
                }
            }
            Command::ConnectManual { multiaddr } => match multiaddr.parse::<Multiaddr>() {
                Ok(addr) => {
                    if let Some(Protocol::P2p(peer)) = addr.iter().last() {
                        self.wanted_peers.insert(peer);
                    }
                    if let Err(err) = self.swarm.dial(addr) {
                        self.emit(Event::Error { message: format!("dial {multiaddr}: {err}") });
                    }
                }
                Err(err) => {
                    self.emit(Event::Error { message: format!("bad multiaddr {multiaddr}: {err}") });
                }
            },
            Command::Punch { peer, reply } => {
                // A punch target is app-relevant by definition; the
                // request-response below dials if not yet connected.
                self.wanted_peers.insert(peer);
                self.announce_peer(peer);
                if let Some(endpoint) = self.ready_endpoint(&peer) {
                    if let Some(reply) = reply {
                        let _ = reply.send(Ok(endpoint));
                    } else {
                        self.emit(Event::PunchResult { peer, ok: true });
                    }
                    return true;
                }
                if let Some(reply) = reply {
                    self.punch_replies.insert(peer, reply);
                }
                self.begin_punch(peer);
            }
            Command::SendDatagram { virtual_ip, src_port, dst_port, data, reliable: _ } => {
                // Broadcast goes to the whole shard rather than one peer --
                // this is what lets a System Link title discover players by
                // shouting instead of naming a host (§17.3.6).
                if self.is_broadcast_vip(virtual_ip) {
                    self.broadcast_datagram(src_port, dst_port, &data);
                    return true;
                }
                let Some(peer) = self.peer_by_vip.get(&virtual_ip) else {
                    self.diagnostics.record("datagram_drop",serde_json::json!({"virtual_ip":virtual_ip,
                        "src_port":src_port,"dst_port":dst_port,"bytes":data.len(),"reason":"unknown_virtual_ip"}));
                    tracing::warn!(virtual_ip, "datagram to unknown virtual IP dropped");
                    return true;
                };
                let peer = *peer;
                if let Some(endpoint) = self.ready_endpoint(&peer) {
                    if let Some(frame) = self.seal_datagram(peer, src_port, dst_port, &data) {
                        self.plane.send_frame(peer, endpoint, &frame, src_port, dst_port);
                    }
                } else if self.tunneled_peers.contains(&peer) {
                    self.send_tunneled(peer, src_port, dst_port, data);
                } else {
                    // Hold it: the punch is usually milliseconds away and this
                    // is typically the opening message of a join.
                    let queue = self.pending_datagrams.entry(peer).or_default();
                    if queue.len() >= PUNCH_QUEUE_MAX {
                        queue.remove(0);
                        self.diagnostics.record("punch_queue_drop",serde_json::json!({"peer":peer.to_base58(),
                            "reason":"queue_full","capacity":PUNCH_QUEUE_MAX}));
                    }
                    queue.push(PendingDatagram {
                        queued_at: Instant::now(),
                        max_age: if self.recovering.contains_key(&peer) { PEER_RECOVERY_RETRY } else { PUNCH_QUEUE_MAX_AGE },
                        src_port,
                        dst_port,
                        data,
                    });
                    tracing::debug!(
                        %peer, dst_port, queued = queue.len(),
                        "datagram held until punch resolves"
                    );
                }
            }
            Command::SetPresence { title_id, state, rich } => {
                self.my_presence.title_id = title_id;
                self.my_presence.state = state;
                self.my_presence.rich = rich;
                self.push_presence(None);
            }
            Command::SetDisplayName { name } => {
                let name = crate::identity::sanitize_display_name(&name);
                self.diagnostics.record("profile_name",serde_json::json!({"name":name}));
                self.display_name = name.clone();
                self.my_presence.display_name = name;
                self.push_presence(None);
            }
            Command::FriendRequest { peer, note } => {
                if self.friend_store.is_friend(&peer) {
                    let display_name = self.friend_store.name(&peer);
                    self.emit(Event::FriendAccepted { peer, display_name });
                    return true;
                }
                self.friend_store.set(peer, Kind::Out, None, Some(&note));
                self.send_friend_request(peer, true);
            }
            Command::FriendAccept { peer } => {
                if self.friend_store.kind(&peer) != Some(Kind::In) {
                    self.emit(Event::Error { message: format!("no pending request from {peer}") });
                    return true;
                }
                self.friend_store.add(peer);
                self.swarm.behaviour_mut().friend.send_request(&peer, FriendMsg::Accept { schema: 1 });
                let display_name = self.friend_store.name(&peer);
                self.emit(Event::FriendAccepted { peer, display_name });
                self.push_presence(Some(peer));
            }
            Command::FriendDecline { peer, block } => {
                self.swarm.behaviour_mut().friend.send_request(&peer, FriendMsg::Remove { schema: 1 });
                if block {
                    self.friend_store.set(peer, Kind::Blocked, None, Some(""));
                    if self.recovering.remove(&peer).is_some() { self.report_peer_departure(peer); }
                } else {
                    self.friend_store.remove(&peer);
                }
            }
            Command::FriendRemove { peer } => {
                self.friend_store.remove(&peer);
                self.swarm.behaviour_mut().friend.send_request(&peer, FriendMsg::Remove { schema: 1 });
                self.emit(Event::FriendRemoved { peer });
            }
            Command::SendInvite { peer, note } => {
                let Some(session) = self.local_session.as_ref() else {
                    self.emit(Event::Error { message: "invite without a local session".into() });
                    return true;
                };
                let expires = std::time::SystemTime::now()
                    .duration_since(std::time::UNIX_EPOCH)
                    .map(|d| d.as_secs() + INVITE_DEFER_MAX.as_secs())
                    .unwrap_or(0);
                let msg = InviteMsg::Offer {
                    schema: 1,
                    title_id: self.title_id,
                    session_id: session.id,
                    expires_unix: expires,
                    note,
                };
                self.invited.insert(peer);
                if self.connected.contains(&peer) {
                    self.swarm.behaviour_mut().invite.send_request(&peer, msg);
                } else {
                    // Not connected yet: hold the invite, dial, and send on
                    // ConnectionEstablished. Also kick a DHT lookup -- we may
                    // not hold an address for this peer at all, which is the
                    // usual reason a first invite fails outright.
                    tracing::info!(%peer, "invite deferred until connected");
                    self.pending_invites.insert(peer, (msg, Instant::now()));
                    self.wanted_peers.insert(peer);
                    if let Err(err) = self.swarm.dial(peer) {
                        tracing::debug!(%peer, %err, "invite dial failed; trying DHT");
                    }
                    self.swarm.behaviour_mut().kad.get_closest_peers(peer);
                }
            }
            Command::InviteReply { peer, accept } => {
                let msg = InviteMsg::Reply { schema: 1, session_id: [0; 16], accept };
                self.swarm.behaviour_mut().invite.send_request(&peer, msg);
                if accept {
                    // Fetch the host's descriptor so the shim can build the
                    // join material (served to invitees even for private
                    // sessions).
                    self.swarm.behaviour_mut().session.send_request(&peer, SessionQuery { schema: 1 });
                }
            }
            Command::RevokeInvite { peer } => {
                self.invited.remove(&peer);
                self.pending_invites.remove(&peer);
                let session_id = self.local_session.as_ref().map_or([0; 16], |s| s.id);
                if self.connected.contains(&peer) {
                    self.swarm.behaviour_mut().invite.send_request(&peer, InviteMsg::Revoke { schema: 1, session_id });
                }
            }
            Command::SessionQuery { peer } => {
                self.wanted_peers.insert(peer);
                self.swarm.behaviour_mut().session.send_request(&peer, SessionQuery { schema: 1 });
            }
            Command::MessageSend { peer, id, blob } => self.queue_envelope(peer, id, 0, blob),
            Command::ClubPublish { team, version, record, members, name } => {
                if record.len() > crate::clubs::MAX_RECORD {
                    self.emit(Event::Error { message: "club record over 64 KB dropped".into() });
                    return true;
                }
                let local = *self.swarm.local_peer_id();
                // Anyone just removed hears this version too, and drops the club.
                let mut targets: HashSet<PeerId> =
                    self.clubs.get(team).map(|c| c.members()).unwrap_or_default().into_iter().collect();
                targets.extend(members.iter().copied());
                let name_key = name.map(|n| crate::protocol::club_name_key(self.title_id, &n));
                if self.clubs.get(team).and_then(|c| c.name_key.clone()) != name_key {
                    self.withdraw_club_name(team);
                }
                self.clubs.set_owned(team, &local, version, record.clone(), &members, name_key, crate::mailbox::unix_now());
                self.advertise_club_names();
                let member_bytes: Vec<Vec<u8>> = members.iter().map(|p| p.to_bytes()).collect();
                for peer in targets {
                    if peer != local && self.connected.contains(&peer) {
                        let push = ClubMsg::Push { schema: 1, team, version, record: record.clone(), members: member_bytes.clone() };
                        self.swarm.behaviour_mut().club.send_request(&peer, push);
                    }
                }
            }
            Command::ClubTrack { team, owner, record } => {
                self.clubs.track(team, &owner, record, crate::mailbox::unix_now());
                self.query_club(team);
            }
            Command::ClubForget { team } => self.clubs.forget(team),
            Command::ClubDelete { team } => {
                self.withdraw_club_name(team);
                let members = self.clubs.get(team).map(|c| c.members()).unwrap_or_default();
                self.clubs.mark_deleted(team, crate::mailbox::unix_now());
                for peer in members {
                    if self.connected.contains(&peer) {
                        self.swarm.behaviour_mut().club.send_request(&peer, ClubMsg::Deleted { schema: 1, team });
                    }
                }
            }
            Command::ClubRequest { owner, id, blob } => self.queue_envelope(owner, id, 1, blob),
            Command::ClubHandover { team, new_owner, id, version, record, members } => {
                self.withdraw_club_name(team);
                self.clubs.set_member(team, &new_owner, version, record.clone(), &members, crate::mailbox::unix_now());
                let handover = Handover { team, version, record, members: members.iter().map(|p| p.to_bytes()).collect() };
                match postcard::to_allocvec(&handover) {
                    Ok(blob) => self.queue_envelope(new_owner, id, 2, blob),
                    Err(err) => tracing::warn!(%err, team, "club handover not encoded"),
                }
            }
            Command::ClubNameCheck { id, name } => {
                let key = crate::protocol::club_name_key(self.title_id, &name);
                let query = self.swarm.behaviour_mut().kad.get_providers(kad::RecordKey::new(&key));
                self.club_name_checks.insert(query, (id, Instant::now()));
            }
            Command::SessionCreate { id, slots_total, slots_open, is_public } => {
                tracing::info!(
                    session_prefix = ?&id[..8], slots_total, slots_open, is_public,
                    "session created"
                );
                self.local_session =
                    Some(LocalSession { id, slots_total, slots_open, is_public, data: Vec::new() });
                if is_public {
                    let key = kad::RecordKey::new(&crate::protocol::session_key(self.title_id));
                    if let Err(err) = self.swarm.behaviour_mut().kad.start_providing(key) {
                        tracing::warn!(%err, "session provider record publish failed");
                    }
                }
            }
            Command::SessionDelete => {
                if let Some(session) = self.local_session.take() {
                    tracing::info!(session_prefix = ?&session.id[..8], "session deleted");
                    if session.is_public {
                        let key =
                            kad::RecordKey::new(&crate::protocol::session_key(self.title_id));
                        self.swarm.behaviour_mut().kad.stop_providing(&key);
                    }
                }
            }
            Command::SessionUpdate { slots_total, slots_open, data } => {
                if let Some(session) = self.local_session.as_mut() {
                    session.slots_total = slots_total;
                    session.slots_open = slots_open;
                    session.data = data;
                }
            }
            Command::EchoProbe { peer } => {
                let (reply, answer) = oneshot::channel();
                let id = self
                    .swarm
                    .behaviour_mut()
                    .echo
                    .send_request(&peer, EchoPayload(b"rexnet-echo".to_vec()));
                self.pending_echo.insert(id, (Instant::now(), reply));
                let emitter = self.emitter.clone();
                tokio::spawn(async move {
                    let rtt_ms = match answer.await {
                        Ok(Ok(rtt)) => Some(u32::try_from(rtt.as_millis()).unwrap_or(u32::MAX)),
                        _ => None,
                    };
                    emitter.emit(Event::EchoResult { peer, rtt_ms });
                });
            }
            Command::ShardEnable { cap } => {
                self.shard_cap = if cap == 0 { DEFAULT_SHARD_CAP } else { cap };
                if !self.shard_enabled {
                    self.shard_enabled = true;
                    tracing::info!(cap = self.shard_cap, "ambient shard enabled");
                    // Look before creating: joining an existing shard is the
                    // whole point, so never create one on the enable path.
                    self.shard_discover();
                }
            }
            Command::ShardDisable => {
                if self.shard_enabled {
                    self.shard_enabled = false;
                    self.shard_leave();
                    self.shard_candidates.clear();
                    self.pending_shard_search = None;
                    self.shard_swept_at = None;
                    tracing::info!("ambient shard disabled");
                }
            }
            Command::StreamConnect { virtual_ip, src_port, dst_port } => {
                let Some(peer) = self.peer_by_vip.get(&virtual_ip).copied() else {
                    tracing::warn!(virtual_ip, "guest TCP connect to unknown virtual IP");
                    self.emit(Event::StreamConnectFailed {
                        virtual_ip,
                        dst_port,
                        message: "unknown virtual IP".into(),
                    });
                    return true;
                };
                let mut control = self.stream_control.clone();
                let events = self.stream_tx.clone();
                let next_id = self.next_stream_id.clone();
                let diagnostics = self.diagnostics.clone();
                tokio::spawn(async move {
                    use libp2p::futures::io::AsyncWriteExt;
                    let protocol = StreamProtocol::new(crate::protocol::TCP);
                    let mut stream = match control.open_stream(peer, protocol).await {
                        Ok(stream) => stream,
                        Err(err) => {
                            let _ = events.send(StreamEvent::ConnectFailed {
                                peer,
                                dst_port,
                                error: err.to_string(),
                            });
                            return;
                        }
                    };
                    // Name the destination port before any guest bytes: the
                    // acceptor cannot match the stream to a listener without it.
                    let header =
                        StreamHeader { schema: STREAM_SCHEMA, dst_port, src_port }.encode();
                    if stream.write_all(&header).await.is_err() || stream.flush().await.is_err() {
                        let _ = events.send(StreamEvent::ConnectFailed {
                            peer,
                            dst_port,
                            error: "header write failed".into(),
                        });
                        return;
                    }
                    let id = next_id.fetch_add(1, Ordering::Relaxed).max(1);
                    let (writer_tx, writer_rx) = mpsc::unbounded_channel::<Vec<u8>>();
                    if events
                        .send(StreamEvent::Opened {
                            id,
                            peer,
                            local_port: src_port,
                            remote_port: dst_port,
                            outbound: true,
                            writer: writer_tx,
                        })
                        .is_err()
                    {
                        return;
                    }
                    pump_stream(id, peer, src_port, dst_port, stream, writer_rx, events, diagnostics).await;
                });
            }
            Command::StreamSend { stream_id, data } => {
                match self.stream_writers.get(&stream_id) {
                    Some(writer) => {
                        if writer.send(data).is_err() {
                            tracing::debug!(stream_id, "guest TCP write to a closed stream");
                        }
                    }
                    None => tracing::debug!(stream_id, "guest TCP write to an unknown stream"),
                }
            }
            Command::StreamClose { stream_id } => {
                // Dropping the writer ends the pump, which reports Closed.
                self.stream_writers.remove(&stream_id);
            }
            Command::ShardStatus { reply } => {
                let _ = reply.send(self.current_shard.as_ref().map(|d| d.shard_id));
            }
            Command::SessionSearch => {
                // Shard members answer in milliseconds; the DHT walk finds
                // everyone else but takes seconds, longer than a title waits.
                let asked: Vec<PeerId> = self.shard_member_peers.union(&self.rexnet_peers).copied().collect();
                tracing::info!(known_members = asked.len(), connected = self.connected.len(),
                    "session search started (known RexNet peers plus DHT)");
                for peer in &asked {
                    self.swarm.behaviour_mut().session.send_request(peer, SessionQuery { schema: 1 });
                }
                if let Some((id, previous)) = &mut self.pending_session_search {
                    // A new three-second menu search must not orphan the DHT
                    // walk started by the previous search: WAN lookups can be slower.
                    for peer in asked {
                        if !previous.contains(&peer) { previous.push(peer); }
                    }
                    tracing::info!(query = ?id, "session search reuses in-flight provider lookup");
                } else {
                    let key = kad::RecordKey::new(&crate::protocol::session_key(self.title_id));
                    let id = self.swarm.behaviour_mut().kad.get_providers(key);
                    self.pending_session_search = Some((id, asked));
                }
            }
            Command::Echo { peer, reply } => {
                let id = self
                    .swarm
                    .behaviour_mut()
                    .echo
                    .send_request(&peer, EchoPayload(b"rexnet-echo".to_vec()));
                self.pending_echo.insert(id, (Instant::now(), reply));
            }
            Command::FindPeer { peer, reply } => {
                let id = self.swarm.behaviour_mut().kad.get_closest_peers(peer);
                self.pending_find.insert(id, (peer, reply));
            }
            Command::LocalAddrs { reply } => {
                let _ = reply.send(self.listen_addrs.clone());
            }
            Command::PublishNotice { signed } => {
                let local = *self.swarm.local_peer_id();
                self.receive_notice(local, signed);
            }
            Command::SendNoticeTo { peer, signed } => {
                self.notice_targets.insert(peer, signed.clone());
                if self.notice_peers.contains(&peer) {
                    self.send_notice(peer, signed, true);
                } else {
                    // The friend lookup path: dial now, and again when the DHT
                    // names the peer (peer_lookups). Identify then delivers.
                    let _ = self.swarm.dial(peer);
                    let key = kad::RecordKey::new(&crate::protocol::peer_key(&peer));
                    let id = self.swarm.behaviour_mut().kad.get_providers(key);
                    self.peer_lookups.insert(id, peer);
                }
            }
            Command::SeedTitle { title_id } => self.title_lookup(title_id, None),
            Command::Roster { title_id, ask, refresh } => {
                self.roster_asked.clear();
                let peers: Vec<PeerId> =
                    self.notice_peers.iter().copied().filter(|peer| refresh || !self.roster_seen.contains(peer)).collect();
                for peer in unasked(&mut self.roster_asked, peers) {
                    self.send_roster_ask(peer, title_id, &ask);
                }
                self.roster_ask = Some((title_id, ask.clone()));
                self.title_lookup(title_id, Some(ask));
            }
            Command::Beacon { title_id } => {
                if self.beacons.insert(title_id) {
                    tracing::info!(title = %format!("{title_id:08X}"), "beaconing");
                    self.provide_beacon(title_id);
                }
            }
            Command::Dial { multiaddr } => match multiaddr.parse::<Multiaddr>() {
                Ok(addr) => {
                    if let Err(err) = self.swarm.dial(addr) {
                        tracing::debug!(%multiaddr, %err, "dial failed");
                    }
                }
                Err(err) => tracing::debug!(%multiaddr, %err, "bad multiaddr"),
            },
            #[cfg(test)]
            Command::DropControlForTest { peer, reply } => {
                self.swarm.behaviour_mut().kad.remove_peer(&peer);
                let _ = self.swarm.disconnect_peer_id(peer);
                let _ = reply.send(());
            }
            #[cfg(test)]
            Command::ControlPathsForTest { peer, reply } => {
                let _ = reply.send(self.peer_links.get(&peer).into_iter().flatten().map(|l| l.quic).collect());
            }
            #[cfg(test)]
            Command::GameSocketForTest { reply } => { let _ = reply.send(self.game_socket.clone()); }
            #[cfg(test)]
            Command::FriendProviderForTest { peer, reply } => {
                let key = kad::RecordKey::new(&crate::protocol::peer_key(&peer));
                let id = self.swarm.behaviour_mut().kad.get_providers(key.clone());
                self.peer_lookups.insert(id, peer);
                // Other providers of this key cannot satisfy a lookup for this identity.
                let other = Keypair::generate_ed25519().public().to_peer_id();
                self.handle_kad_result(id, kad::QueryResult::GetProviders(Ok(kad::GetProvidersOk::FoundProviders {
                    key: key.clone(), providers: HashSet::from([other]),
                })), false, 1);
                let retained = self.peer_lookups.contains_key(&id);
                for _ in 0..4 {
                    self.handle_kad_result(id, kad::QueryResult::GetProviders(Ok(kad::GetProvidersOk::FoundProviders {
                        key: key.clone(), providers: HashSet::from([peer]),
                    })), false, 1);
                }
                let _ = reply.send((retained, self.peer_lookups.contains_key(&id)
                    || self.swarm.behaviour().kad.query(&id).is_some()));
            }
            Command::Shutdown => return false,
        }
        true
    }

    fn handle_swarm_event(&mut self, event: SwarmEvent<BehaviourEvent>) {
        match event {
            SwarmEvent::NewListenAddr { address, .. } => {
                tracing::info!(%address, "listening");
                self.listen_addrs.push(address);
            }
            SwarmEvent::ExternalAddrConfirmed { address } => {
                tracing::info!(%address, "external address confirmed");
                let new = self.confirmed_addrs.insert(address.clone());
                self.note_external_addr(address);
                // Our record should carry the new address: republish next heartbeat.
                self.own_key_at = None;
                // A beacon's too, at once: a relay circuit is how runners reach a
                // panel behind NAT. Only for a new address, since AutoNAT
                // re-confirms and each announcement is an Amino AddProvider walk.
                if new {
                    self.provide_beacons();
                }
            }
            // A circuit that comes back after its reservation lapsed is new again.
            SwarmEvent::ExternalAddrExpired { address } => {
                self.confirmed_addrs.remove(&address);
            }
            SwarmEvent::ListenerClosed { listener_id, reason, .. } => {
                self.auto_relay_listener_closed(listener_id, reason.is_err());
            }
            SwarmEvent::ConnectionEstablished { peer_id, connection_id, endpoint, num_established, .. } => {
                // Debug, not info: the public DHT crawl connects to hundreds of
                // peers that have nothing to do with us, and at 50% of the log
                // it buries everything that matters. A peer becomes interesting
                // when identify shows it speaks our protocols -- that is logged
                // at info instead.
                tracing::debug!(%peer_id, endpoint = %endpoint.get_remote_address(), "connected");
                if self.peer_routes.contains_key(&peer_id) || self.announced.contains(&peer_id)
                    || self.shard_member_peers.contains(&peer_id) || self.wanted_peers.contains(&peer_id) {
                    self.remember_peer_routes(peer_id, reconnect_address(&endpoint));
                }
                let relayed = endpoint.is_relayed();
                let budget = if !relayed {
                    (None, None)
                } else {
                    let key = if endpoint.is_dialer() { circuit_relay(endpoint.get_remote_address()) } else { Some(peer_id) };
                    // Unreported: assume a public relay's rendezvous budget.
                    key.and_then(|key| self.circuit_limits.get(&key).copied()).unwrap_or_else(|| {
                        let public = relay::Config::default();
                        (Some(public.max_circuit_duration), Some(public.max_circuit_bytes))
                    })
                };
                let (now, bytes) = (Instant::now(), self.tunnel_bytes.get(&peer_id).copied().unwrap_or(0));
                self.peer_links.entry(peer_id).or_default().push(Link {
                    id: connection_id,
                    relayed,
                    quic: endpoint.get_remote_address().iter().any(|p| matches!(p, Protocol::QuicV1)),
                    renew_at: budget.0.map(|time| now + time / 2),
                    renew_bytes: budget.1.map(|data| bytes + data / 2),
                    remote: endpoint.is_dialer().then(|| endpoint.get_remote_address().clone()),
                    inbound: endpoint.is_listener(),
                });
                // The other end may detect a broken QUIC route first and
                // reconnect over TCP or a relay while our old QUIC link still looks live.
                // Retire that link so stream selection cannot keep using it.
                let old_quic = quic_links_replaced(&self.peer_links[&peer_id], endpoint.get_remote_address(),
                    self.tunneled_peers.contains(&peer_id) || self.recovering.contains_key(&peer_id));
                let replacing_quic = !old_quic.is_empty();
                for id in old_quic { self.swarm.close_connection(id); }
                if self.shard_member_peers.contains(&peer_id) {
                    tracing::info!(%peer_id, %connection_id, relayed, circuit_seconds = ?budget.0.map(|t| t.as_secs()),
                        circuit_bytes = ?budget.1, num_established, "member link established");
                    if !replacing_quic { self.retire_superseded_links(peer_id); }
                }
                if num_established.get() == 1 {
                    let wanted = self.wanted_peers.remove(&peer_id);
                    self.connected.insert(peer_id);
                    self.shard_retry_at.remove(&peer_id);
                    self.friend_retry_at.remove(&peer_id);
                    if self.shard_member_peers.contains(&peer_id) || wanted || self.announced.contains(&peer_id) {
                        tracing::info!(%peer_id, ?endpoint, "control connection established");
                    }
                    // Peers we didn't ask for (kad crawl, autonat dial-backs)
                    // stay unannounced until they speak a rexnet protocol.
                    if wanted || self.friend_store.is_friend(&peer_id) {
                        self.announce_peer(peer_id);
                    }
                    if self.friend_store.is_friend(&peer_id) {
                        self.push_presence(Some(peer_id));
                    }
                    // A request we made while they were away: send it again.
                    if self.friend_store.kind(&peer_id) == Some(Kind::Out) {
                        self.send_friend_request(peer_id, false);
                    }
                    self.deliver_messages(peer_id);
                    let owned_by: Vec<u64> = self
                        .clubs
                        .clubs()
                        .iter()
                        .filter(|c| c.role == crate::clubs::Role::Member && c.owner() == Some(peer_id))
                        .map(|c| c.team)
                        .collect();
                    for team in owned_by {
                        self.query_club(team);
                    }
                    // A shard member just became connected: get its game plane
                    // up now rather than when the player tries to join.
                    if self.shard_member_peers.contains(&peer_id) {
                        self.shard_warm_game_plane(peer_id);
                    }
                    if self.recovering.contains_key(&peer_id) {
                        // The normal six-second punch timeout exceeds the recovery
                        // window. Resume over the authenticated tunnel at once;
                        // a simultaneous fresh punch may still upgrade to UDP.
                        self.begin_punch(peer_id);
                        self.tunnel_fallback(peer_id);
                    }
                    // Deliver any invite that was waiting on this connection,
                    // unless the user has long since moved on.
                    if let Some((msg, queued)) = self.pending_invites.remove(&peer_id) {
                        if queued.elapsed() <= INVITE_DEFER_MAX {
                            tracing::info!(%peer_id, "sending deferred invite");
                            self.announce_peer(peer_id);
                            self.swarm.behaviour_mut().invite.send_request(&peer_id, msg);
                        } else {
                            tracing::info!(
                                %peer_id, "dropping stale deferred invite"
                            );
                        }
                    }
                }
            }
            SwarmEvent::ConnectionClosed { peer_id, connection_id, endpoint, num_established, cause, .. } => {
                if self.announced.contains(&peer_id) || self.shard_member_peers.contains(&peer_id) {
                    let fields = serde_json::json!({"peer":peer_id.to_base58(),"connection_id":connection_id.to_string(),
                        "endpoint":format!("{endpoint:?}"),"remaining_connections":num_established,
                        "cause":format!("{cause:?}"),"direct_ready":self.game_plane_ready(&peer_id),
                        "tunnel":self.tunneled_peers.contains(&peer_id)});
                    if num_established == 0 { self.diagnostics.incident("peer_disconnected",fields); }
                    else { self.diagnostics.record("control_link_closed",fields); }
                }
                if let Some(links) = self.peer_links.get_mut(&peer_id) {
                    links.retain(|link| link.id != connection_id);
                    if links.is_empty() {
                        self.peer_links.remove(&peer_id);
                    }
                }
                if self.rexnet_peers.contains(&peer_id) || self.announced.contains(&peer_id) {
                    tracing::info!(%peer_id, %connection_id, ?endpoint, num_established, ?cause,
                        game_direct = self.game_plane_ready(&peer_id),
                        game_tunnel = self.tunneled_peers.contains(&peer_id),
                        "control connection closed");
                }
                if num_established == 0 {
                    let game_peer = self.announced.contains(&peer_id)
                        && self.friend_store.kind(&peer_id) != Some(Kind::Blocked)
                        && (self.recovering.contains_key(&peer_id) || self.recovering.len() < ROUTE_CACHE_PEERS)
                        && (self.tunneled_peers.contains(&peer_id) || self.game_plane_ready(&peer_id)
                            || self.punch_deadline.contains_key(&peer_id) || self.recovering.contains_key(&peer_id));
                    if game_peer {
                        self.remember_peer_routes(peer_id, reconnect_address(&endpoint));
                        let now = Instant::now();
                        if direct_quic(endpoint.get_remote_address()) {
                            if let Some(routes) = self.peer_routes.get_mut(&peer_id) {
                                routes.avoid_quic_until = Some(now + SHARD_RETRY_MAX);
                            }
                        }
                        if !self.recovering.contains_key(&peer_id) {
                            self.recovering.insert(peer_id, PeerRecovery { started: now, next_dial: now, lookup: None });
                            self.diagnostics.record("game_peer_recovery_started", serde_json::json!({"peer":peer_id.to_base58(),
                                "grace_ms":PEER_RECOVERY_GRACE.as_millis(),"failed_endpoint":format!("{endpoint:?}")}));
                            tracing::info!(%peer_id, "game peer transport lost; attempting bounded recovery");
                        }
                    }
                    // Only worth a line if we ever cared about this peer.
                    if self.rexnet_peers.contains(&peer_id) || self.announced.contains(&peer_id) {
                        tracing::info!(%peer_id, ?cause, "rexnet peer disconnected");
                    } else {
                        tracing::debug!(%peer_id, ?cause, "disconnected");
                    }
                    self.connected.remove(&peer_id);
                    self.pending_keepalive.retain(|_, (peer, _)| *peer != peer_id);
                    // Virtual IP stays allocated (stable across reconnects);
                    // the punched endpoint is dropped as stale.
                    self.plane.unmap_endpoint(&peer_id);
                    // A relay that goes away stops being one; leaving it
                    // labelled would strand a stale "relay" row in the UI.
                    self.rexnet_peers.remove(&peer_id);
                    self.notice_peers.remove(&peer_id);
                    self.relay_peers.lock().unwrap_or_else(|error| error.into_inner()).remove(&peer_id);
                    self.pending_datagrams.remove(&peer_id);
                    // Guest TCP sockets to a peer that went away must be
                    // closed, or the game waits on a socket that will never
                    // produce anything again.
                    for id in self.streams.streams_for_peer(&peer_id) {
                        self.streams.remove(id);
                        self.stream_writers.remove(&id);
                        self.emit(Event::StreamClosed { stream_id: id });
                    }
                    self.punch_deadline.remove(&peer_id);
                    // Nonces are only ever added, so this is where they leave.
                    self.pending_punch_nonce.retain(|_, p| *p != peer_id);
                    self.pending_punch_offers.retain(|_, p| *p != peer_id);
                    self.tunneled_peers.remove(&peer_id);
                    self.close_tunnel(&peer_id);
                    // A reconnect negotiates fresh keys, so counters and the
                    // replay window are never reused.
                    self.peer_rtt_ms.remove(&peer_id);
                    self.key_agreements.remove(&peer_id);
                    self.peer_game_pubs.remove(&peer_id);
                    self.plane.remove_keys(&peer_id);
                    // Allow a retry if they come back on the topic.
                    self.shard_retry_at.remove(&peer_id);
                    if self.relays.remove(&peer_id) {
                        self.emit(Event::RelayStatus { peer: peer_id, is_relay: false });
                    }
                    // Free the auto-relay slot so another reachable peer can take
                    // its place next time one identifies.
                    self.auto_relays.remove(&peer_id);
                    if game_peer { self.recover_peer(peer_id); }
                    else {
                        self.recovering.remove(&peer_id);
                        self.report_peer_departure(peer_id);
                    }
                } else if direct_quic(endpoint.get_remote_address()) && self.tunneled_peers.contains(&peer_id) {
                    // Both directions need a fresh writer after the old link
                    // closes, even if the replacement arrived before our timeout.
                    self.close_tunnel(&peer_id);
                    self.tunnel_writer(peer_id);
                }
            }
            SwarmEvent::OutgoingConnectionError { peer_id, error, .. } => {
                if let Some(peer) = peer_id { self.lookup_recovery_peer(peer); }
                let wanted = peer_id.map_or(true, |p| self.wanted_peers.remove(&p));
                let shard_member = peer_id.is_some_and(|p| self.shard_member_peers.contains(&p));
                // A failed dial only matters if we asked for that peer. The
                // kad crawl fails against unreachable public nodes constantly
                // and each one was printing a multi-line transport error.
                if wanted || shard_member {
                    tracing::warn!(?peer_id, ?error, "outgoing connection failed; known members will retry");
                } else {
                    tracing::debug!(?peer_id, %error, "dial failed");
                }
                if wanted {
                    self.emit(Event::Error { message: format!("connect {peer_id:?}: {error}") });
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Identify(identify::Event::Received {
                peer_id,
                info,
                ..
            })) => {
                // A peer that speaks our shard protocol is another RexNet
                // node. Worth knowing separately from `connected`, which is
                // mostly public DHT peers we will never ask anything.
                let rexnet = info.protocols.iter().any(|p| p.as_ref() == crate::protocol::SHARD);
                if rexnet || self.announced.contains(&peer_id) {
                    self.remember_peer_routes(peer_id, info.listen_addrs.iter().cloned());
                    if let Some(links) = self.peer_links.get(&peer_id) {
                        let routes: Vec<_> = links.iter().filter_map(|link| link.remote.clone()).collect();
                        self.remember_peer_routes(peer_id, routes);
                    }
                }
                if rexnet {
                    if self.rexnet_peers.insert(peer_id) {
                        self.relay_peers.lock().unwrap_or_else(|error| error.into_inner()).insert(peer_id);
                        // Whether it dialled us: how a beacon's reach is measured.
                        let inbound = self.peer_links.get(&peer_id).is_some_and(|links| links.iter().any(|link| link.inbound));
                        tracing::info!(%peer_id, inbound, "rexnet peer identified");
                        // Late arrivals matter: if we already hold a shard and
                        // this peer holds an older one, we want to find out now
                        // rather than at the next sweep.
                        if self.shard_enabled {
                            self.shard_query_peer(peer_id);
                        }
                        // A roster round is under way (panel only): ask now,
                        // not at the next round. An old build refuses the
                        // protocol and is reported as one.
                        if let Some((title_id, ask)) = self.roster_ask.clone() {
                            if !unasked(&mut self.roster_asked, [peer_id]).is_empty() {
                                self.send_roster_ask(peer_id, title_id, &ask);
                            }
                        }
                    }
                }
                // Independent of the shard check: the panel speaks notices
                // without ever joining a shard.
                if info.protocols.iter().any(|p| p.as_ref() == crate::protocol::NOTICE) {
                    self.notice_peer_identified(peer_id);
                }
                // Torrent-model relaying: if this peer offers the circuit-v2
                // hop on a routable address, hold a reservation on it so a NAT'd
                // node gets a relay from the mesh itself, no configured server.
                self.maybe_auto_reserve_relay(peer_id, &info.protocols, &info.listen_addrs);
                // After the relay pick, so a relay it just chose is kept.
                if !rexnet {
                    self.close_public_inbound(peer_id);
                }
                // Feed identify-learned addresses into the routing table so
                // kad queries and dials can use them.
                for addr in info.listen_addrs {
                    self.swarm.behaviour_mut().kad.add_address(&peer_id, addr);
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Mdns(mdns::Event::Discovered(peers))) => {
                for (peer, addr) in peers {
                    tracing::info!(%peer, %addr, "mdns discovered");
                    self.swarm.behaviour_mut().kad.add_address(&peer, addr);
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Kad(kad::Event::OutboundQueryProgressed {
                id,
                result,
                step,
                stats,
            })) => self.handle_kad_result(id, result, step.last, stats.num_successes()),
            SwarmEvent::Behaviour(BehaviourEvent::Echo(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    tracing::debug!(%peer, "echo request");
                    self.announce_peer(peer);
                    let _ = self.swarm.behaviour_mut().echo.send_response(channel, request);
                }
                request_response::Message::Response { request_id, .. } => {
                    if let Some((peer, start)) = self.pending_keepalive.remove(&request_id) {
                        tracing::debug!(%peer, rtt_ms = start.elapsed().as_millis(), "control keepalive answered");
                    }
                    if let Some((start, reply)) = self.pending_echo.remove(&request_id) {
                        let _ = reply.send(Ok(start.elapsed()));
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Echo(
                request_response::Event::OutboundFailure { request_id, error, .. },
            )) => {
                if let Some((peer, start)) = self.pending_keepalive.remove(&request_id) {
                    tracing::warn!(%peer, elapsed_ms = start.elapsed().as_millis(), ?error, "control keepalive failed");
                }
                if let Some((_, reply)) = self.pending_echo.remove(&request_id) {
                    let _ = reply.send(Err(error.to_string()));
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Session(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request_id, channel, .. } => {
                    // Serve our descriptor to queriers. Private sessions are
                    // only served to peers we invited (they never touch the
                    // DHT either).
                    let invited = self.invited.contains(&peer);
                    let friend = self.friend_store.is_friend(&peer);
                    let desc = self
                        .local_session
                        .as_ref()
                        .filter(|s| s.is_public || invited || friend)
                        .map(|s| session_desc(s, self.title_id));
                    tracing::info!(%peer, %request_id, session_present = desc.is_some(),
                        invited, friend, "session query received; replying");
                    self.emit(Event::SessionQueried { peer, served: desc.is_some() });
                    if self.swarm.behaviour_mut().session.send_response(channel, desc).is_err() {
                        tracing::warn!(%peer, %request_id, "session response channel already closed");
                    }
                }
                request_response::Message::Response { request_id, response } => {
                    tracing::info!(%peer, %request_id, session_present = response.is_some(),
                        "session query response received");
                    if let Some(desc) = response {
                        if desc.schema == 1 && desc.title_id == self.title_id {
                            // Surface the host before SessionFound so the
                            // shim's vip table has it when it builds the
                            // search result / join material.
                            self.announce_peer(peer);
                            self.emit(Event::SessionFound {
                                host: peer,
                                session_id: desc.session_id,
                                slots_total: desc.slots_total,
                                slots_open: desc.slots_open,
                                requires_invite: desc.requires_invite,
                                data: desc.data,
                            });
                        } else {
                            tracing::info!(%peer, schema = desc.schema, title = desc.title_id,
                                expected_title = self.title_id, "session response rejected: schema or title mismatch");
                        }
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Session(
                request_response::Event::OutboundFailure { peer, request_id, error, .. },
            )) => tracing::warn!(%peer, %request_id, ?error, "session query failed"),
            SwarmEvent::Behaviour(BehaviourEvent::Session(
                request_response::Event::InboundFailure { peer, request_id, error, .. },
            )) => tracing::warn!(%peer, %request_id, ?error, "session query inbound failed"),
            SwarmEvent::Behaviour(BehaviourEvent::Presence(request_response::Event::Message {
                peer,
                message,
                ..
            })) => {
                if let request_response::Message::Request { request, channel, .. } = message {
                    let _ =
                        self.swarm.behaviour_mut().presence.send_response(channel, Ack { schema: 1 });
                    let local = *self.swarm.local_peer_id();
                    let club = self.clubs.knows(&peer, &local);
                    // Protocol-enforced: presence is dropped unless the sender
                    // is a mutual friend (§8.1) or shares a club with us.
                    if request.schema == 1 && (self.friend_store.is_friend(&peer) || club) {
                        self.friend_store.set_name(&peer, &request.display_name);
                        self.announce_peer(peer);
                        self.emit(Event::PresenceUpdated {
                            peer,
                            title_id: request.title_id,
                            state: request.state,
                            display_name: request.display_name,
                            rich: request.rich,
                        });
                    } else if !club && self.friend_store.kind(&peer) != Some(Kind::Out) {
                        // They still list us, but we removed or declined them
                        // while they were away: tell them now. An `Out` peer
                        // may be presence-before-accept, so it is spared.
                        self.swarm.behaviour_mut().friend.send_request(&peer, FriendMsg::Remove { schema: 1 });
                    }
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Friend(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    let ack = FriendAck { schema: 1, display_name: self.display_name.clone() };
                    let _ = self.swarm.behaviour_mut().friend.send_response(channel, ack);
                    self.handle_friend_msg(peer, request);
                }
                request_response::Message::Response { request_id, response } => {
                    if self.pending_friend_requests.remove(&request_id).is_some() {
                        self.friend_store.set_name(&peer, &response.display_name);
                        // Still waiting for their answer (a mutual request may
                        // already have made us friends).
                        if self.friend_store.kind(&peer) == Some(Kind::Out) {
                            let display_name = self.friend_store.name(&peer);
                            self.emit(Event::FriendRequestSent { peer, display_name });
                        }
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Friend(request_response::Event::OutboundFailure {
                peer,
                request_id,
                error,
                ..
            })) => {
                if let Some((_, asked)) = self.pending_friend_requests.remove(&request_id) {
                    tracing::info!(%peer, ?error, asked, "friend request undelivered");
                    if asked && self.friend_store.kind(&peer) == Some(Kind::Out) {
                        self.friend_store.remove(&peer);
                        self.emit(Event::FriendRequestFailed { peer });
                    }
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Club(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    let reply = self.handle_club_msg(peer, request);
                    let _ = self.swarm.behaviour_mut().club.send_response(channel, reply);
                }
                request_response::Message::Response { request_id, response } => {
                    if let Some(team) = self.club_queries.remove(&request_id) {
                        self.handle_club_reply(peer, team, response);
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Message(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    let _ = self.swarm.behaviour_mut().message.send_response(channel, Ack { schema: 1 });
                    if self.friend_store.kind(&peer) == Some(Kind::Blocked) {
                        tracing::info!(%peer, "message from blocked peer dropped");
                    } else if request.blob.len() > crate::mailbox::MAX_BLOB
                        || !self.mailbox.first_sight(request.id, crate::mailbox::unix_now())
                    {
                        tracing::info!(%peer, "duplicate or oversized message dropped");
                    } else {
                        self.announce_peer(peer);
                        match request.kind {
                            0 => self.emit(Event::MessageReceived { peer, blob: request.blob }),
                            1 => self.emit(Event::ClubRequest { peer, blob: request.blob }),
                            2 => self.receive_handover(peer, &request.blob),
                            kind => tracing::info!(%peer, kind, "envelope of an unknown kind dropped"),
                        }
                    }
                }
                request_response::Message::Response { request_id, .. } => {
                    if let Some((peer, id)) = self.messages_in_flight.remove(&request_id) {
                        tracing::info!(%peer, "message delivered");
                        self.mailbox.delivered(&peer, id);
                        // A delivered handover releases what waited behind it.
                        self.deliver_messages(peer);
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Message(request_response::Event::OutboundFailure {
                peer,
                request_id,
                error,
                ..
            })) => {
                if self.messages_in_flight.remove(&request_id).is_some() {
                    tracing::info!(%peer, ?error, "message undelivered; kept for the next connection");
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Invite(request_response::Event::Message {
                peer,
                message,
                ..
            })) => {
                if let request_response::Message::Request { request, channel, .. } = message {
                    let _ =
                        self.swarm.behaviour_mut().invite.send_response(channel, Ack { schema: 1 });
                    match request {
                        InviteMsg::Offer { title_id, session_id, expires_unix, note, .. } => {
                            let now = std::time::SystemTime::now()
                                .duration_since(std::time::UNIX_EPOCH)
                                .map(|d| d.as_secs())
                                .unwrap_or(0);
                            if expires_unix != 0 && now > expires_unix {
                                tracing::debug!(%peer, "expired invite dropped");
                            } else {
                                self.announce_peer(peer);
                                self.emit(Event::InviteReceived { peer, title_id, session_id, note });
                            }
                        }
                        InviteMsg::Reply { accept, .. } => {
                            self.emit(Event::InviteReplied { peer, reply: accept as u8 });
                        }
                        InviteMsg::Revoke { session_id, .. } => self.emit(Event::InviteRevoked { peer, session_id }),
                    }
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Punch(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    // Responder half of §8.4: answer with our candidates
                    // under the offer's nonce, then probe toward theirs.
                    tracing::info!(
                        %peer,
                        theirs = ?request.candidates,
                        "punch offer received"
                    );
                    self.announce_peer(peer);
                    let candidates = self.local_candidates();
                    let nonce = request.nonce;
                    let correlation=nonce.iter().map(|b|format!("{b:02x}")).collect::<String>();
                    self.diagnostics.record("punch_offer_received",serde_json::json!({"peer":peer.to_base58(),"correlation":correlation,"candidates":request.candidates}));
                    self.pending_punch_nonce.insert(nonce, peer);
                    let eph_pub = self.local_game_pubkey(peer);
                    self.establish_game_keys(peer, request.eph_pub);
                    self.diagnostics.record("punch_answer_sent",serde_json::json!({"peer":peer.to_base58(),"correlation":correlation,"candidates":candidates}));
                    let _ = self
                        .swarm
                        .behaviour_mut()
                        .punch
                        .send_response(channel, PunchAnswer { nonce, candidates, eph_pub });
                    self.spawn_probes(nonce, request.candidates);
                }
                request_response::Message::Response { request_id, response } => {
                    if self.pending_punch_offers.remove(&request_id).is_some() {
                        self.diagnostics.record("punch_answer_received",serde_json::json!({"peer":peer.to_base58(),
                            "correlation":response.nonce.iter().map(|b|format!("{b:02x}")).collect::<String>(),"candidates":response.candidates}));
                        self.establish_game_keys(peer, response.eph_pub);
                        self.spawn_probes(response.nonce, response.candidates);
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Punch(
                request_response::Event::OutboundFailure { request_id, error, .. },
            )) => {
                if let Some(peer) = self.pending_punch_offers.remove(&request_id) {
                    self.emit(Event::PunchResult { peer, ok: false });
                    if let Some(reply) = self.punch_replies.remove(&peer) {
                        let _ = reply.send(Err(error.to_string()));
                    }
                    // The offer itself never landed, so no probes are coming.
                    // No reason to sit out the full timeout.
                    self.punch_deadline.remove(&peer);
                    self.tunnel_fallback(peer);
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Shard(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    // Answer with our shard only if it is for the title being
                    // asked about; a peer may run a different game.
                    let desc = self
                        .current_shard
                        .as_ref()
                        .filter(|d| d.title_id == request.title_id)
                        .map(|d| {
                            let mut d = d.clone();
                            // Publish the live count, not the stale creation
                            // value -- this is what other peers place against.
                            d.members_hint = self.shard_member_count();
                            d
                        });
                    let _ = self.swarm.behaviour_mut().shard.send_response(channel, desc);
                }
                request_response::Message::Response { request_id, response, .. } => {
                    self.pending_shard_queries.remove(&request_id);
                    if let Some(desc) = response {
                        if desc.title_id == self.title_id && desc.schema == 1 {
                            tracing::info!(
                                %peer, shard = %hex16(&desc.shard_id),
                                members = desc.members_hint, "shard descriptor received"
                            );
                            self.shard_candidates.insert(desc.shard_id, desc);
                        }
                    }
                    // Act the moment the picture is complete. Waiting for the
                    // next rescan would leave a joining player shardless for
                    // up to a full interval for no reason.
                    if self.pending_shard_queries.is_empty() {
                        self.shard_settle();
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Shard(
                request_response::Event::OutboundFailure { request_id, error, .. },
            )) => {
                // An unreachable advertiser must not stall placement -- its
                // record may be stale long after it left.
                if let Some(peer) = self.pending_shard_queries.remove(&request_id) {
                    tracing::debug!(%peer, ?error, "shard query failed (advertisement may be stale)");
                }
                if self.pending_shard_queries.is_empty() {
                    self.shard_settle();
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Notice(request_response::Event::Message {
                peer,
                message,
                ..
            })) => match message {
                request_response::Message::Request { request, channel, .. } => {
                    let reply = match request {
                        NoticeRequest::Notice(signed) => {
                            self.receive_notice(peer, signed);
                            NoticeReply::Ack
                        }
                        NoticeRequest::Ask(signed) => {
                            let valid = crate::notice::ask_is_valid(
                                &self.notice_key, &signed, &peer, crate::notice::unix_ms());
                            tracing::debug!(%peer, valid, "roster ask received");
                            NoticeReply::Identity(valid.then(|| RosterIdentity {
                                display_name: self.display_name.clone(),
                                title_id: self.title_id,
                            }))
                        }
                    };
                    let _ = self.swarm.behaviour_mut().notice.send_response(channel, reply);
                }
                request_response::Message::Response { request_id, response, .. } => {
                    let asked = self.roster_asks.remove(&request_id).is_some();
                    match response {
                        NoticeReply::Ack => {
                            if let Some(peer) = self.notice_sends.remove(&request_id) {
                                self.notice_targets.remove(&peer);
                                self.emit(Event::NoticeDelivered { peer });
                            }
                        }
                        NoticeReply::Identity(Some(identity)) if asked => {
                            self.roster_seen.insert(peer);
                            self.emit(Event::RosterEntry {
                                peer,
                                title_id: identity.title_id,
                                display_name: crate::identity::sanitize_display_name(&identity.display_name),
                                addr: roster_addr(self.peer_links.get(&peer).map_or(&[], Vec::as_slice), peer),
                            });
                        }
                        NoticeReply::Identity(_) => {}
                    }
                }
            },
            SwarmEvent::Behaviour(BehaviourEvent::Notice(request_response::Event::OutboundFailure {
                peer,
                request_id,
                error,
                ..
            })) => {
                // A targeted notice stays in notice_targets; the panel resends it.
                self.notice_sends.remove(&request_id);
                if let Some(title_id) = self.roster_asks.remove(&request_id) {
                    if let Some(event) = old_build_event(&error, peer, title_id) {
                        self.roster_seen.insert(peer);
                        self.emit(event);
                    }
                }
                tracing::debug!(%peer, ?error, "notice request failed");
            }
            SwarmEvent::Behaviour(BehaviourEvent::Gossipsub(gossipsub::Event::Message {
                message,
                ..
            })) => {
                // Trust the gossipsub source over anything in the payload:
                // ShardBeat carries no identity of its own, precisely so a
                // peer cannot claim to be someone else.
                let Some(source) = message.source else { return };
                match postcard::from_bytes::<ShardBeat>(&message.data) {
                    Ok(beat) if beat.schema == 1 => {
                        self.shard_member_seen(source);
                        self.emit(Event::ShardPresence {
                            peer: source,
                            state: beat.state,
                            rich: beat.rich,
                            session_id: beat.session_id,
                        });
                    }
                    Ok(_) => tracing::debug!(%source, "shard beat: unknown schema"),
                    Err(err) => tracing::debug!(%source, %err, "shard beat: malformed"),
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::Gossipsub(gossipsub::Event::Subscribed {
                peer_id,
                topic,
            })) => {
                if self.current_shard.as_ref().is_some_and(|current| {
                    topic
                        == gossipsub::IdentTopic::new(crate::protocol::shard_topic(&current.shard_id))
                            .hash()
                }) {
                    self.shard_member_seen(peer_id);
                }
            }
            SwarmEvent::Behaviour(BehaviourEvent::RelayClient(
                relay::client::Event::ReservationReqAccepted { relay_peer_id, renewal, limit },
            )) => {
                tracing::info!(%relay_peer_id, renewal, ?limit, "relay reservation accepted");
                self.mark_relay(relay_peer_id, true);
            },
            SwarmEvent::Behaviour(BehaviourEvent::RelayClient(
                relay::client::Event::OutboundCircuitEstablished { relay_peer_id, limit },
            )) => {
                tracing::info!(%relay_peer_id, ?limit, "outbound relay circuit established");
                self.circuit_limits.insert(relay_peer_id,
                    limit.as_ref().map_or((None, None), |l| (l.duration(), l.data_in_bytes())));
                self.mark_relay(relay_peer_id, true);
            },
            SwarmEvent::Behaviour(BehaviourEvent::RelayClient(
                relay::client::Event::InboundCircuitEstablished { src_peer_id, limit },
            )) => {
                tracing::info!(%src_peer_id, ?limit, "inbound relay circuit established");
                self.circuit_limits.insert(src_peer_id,
                    limit.as_ref().map_or((None, None), |l| (l.duration(), l.data_in_bytes())));
            }
            // We are serving as someone's relay -- the "seed" half of the mesh.
            // Logged at info because it is exactly the behaviour a player wants
            // to confirm ("am I helping others connect?").
            SwarmEvent::Behaviour(BehaviourEvent::RelayServer(event)) => {
                tracing::info!(?event, "relay server (we are relaying for the mesh)");
            }
            SwarmEvent::Behaviour(BehaviourEvent::Dcutr(event)) => {
                tracing::info!(?event, "dcutr");
            }
            SwarmEvent::Behaviour(BehaviourEvent::Autonat(event)) => {
                tracing::debug!(?event, "autonat");
            }
            SwarmEvent::Behaviour(BehaviourEvent::Upnp(event)) => match event {
                upnp::Event::NewExternalAddr(addr) => {
                    // The router mapped our port: this is a directly dialable
                    // address. The swarm records it as an external address on
                    // its own; we log and surface it so the player can hand the
                    // bare host:port to a peer.
                    tracing::info!(%addr, "upnp: external address mapped");
                    self.note_external_addr(addr);
                }
                upnp::Event::ExpiredExternalAddr(addr) => {
                    tracing::info!(%addr, "upnp: external address expired");
                }
                upnp::Event::GatewayNotFound => {
                    tracing::info!("upnp: no IGD gateway found (router has UPnP off or absent)");
                }
                upnp::Event::NonRoutableGateway => {
                    tracing::info!("upnp: gateway is not internet-routable (CGNAT/double NAT)");
                }
            },
            other => tracing::trace!(?other, "swarm event"),
        }
    }

    fn handle_kad_result(&mut self, id: kad::QueryId, result: kad::QueryResult, last: bool, successes: u32) {
        match result {
            kad::QueryResult::GetClosestPeers(res) => {
                let recovery_peer = self.recovering.iter()
                    .find(|(_, r)| r.lookup == Some(id)).map(|(peer, _)| *peer);
                if let Some(peer) = recovery_peer {
                    let peers = match &res {
                        Ok(ok) => &ok.peers,
                        Err(kad::GetClosestPeersError::Timeout { peers, .. }) => peers,
                    };
                    if let Some(found) = peers.iter().find(|p| p.peer_id == peer) {
                        self.remember_peer_routes(peer, found.addrs.iter().cloned());
                        self.recovering.get_mut(&peer).unwrap().next_dial = Instant::now();
                        self.recover_peer(peer);
                    }
                }
                let Some((target, _)) = self.pending_find.get(&id) else { return };
                let target = *target;
                let found = match &res {
                    Ok(ok) => ok.peers.iter().find(|p| p.peer_id == target).cloned(),
                    Err(kad::GetClosestPeersError::Timeout { peers, .. }) => {
                        peers.iter().find(|p| p.peer_id == target).cloned()
                    }
                };
                if let Some(info) = found {
                    if let Some((_, reply)) = self.pending_find.remove(&id) {
                        let _ = reply.send(Ok(info.addrs));
                    }
                } else if last {
                    if let Some((_, reply)) = self.pending_find.remove(&id) {
                        let _ = reply.send(Err(format!("peer {target} not found in DHT")));
                    }
                }
            }
            kad::QueryResult::GetProviders(res) => {
                if let Some(&(check, _)) = self.club_name_checks.get(&id) {
                    let local = *self.swarm.local_peer_id();
                    let taken = matches!(&res, Ok(kad::GetProvidersOk::FoundProviders { providers, .. })
                        if providers.iter().any(|p| *p != local));
                    if taken || last {
                        self.club_name_checks.remove(&id);
                        self.emit(Event::ClubNameResult { id: check, taken });
                    }
                    return;
                }
                if self.seed_searches.contains_key(&id) || self.roster_searches.contains_key(&id) {
                    let local = *self.swarm.local_peer_id();
                    let roster = self.roster_searches.get(&id).cloned();
                    if let Ok(kad::GetProvidersOk::FoundProviders { providers, .. }) = &res {
                        let others = providers.iter().copied().filter(|peer| *peer != local);
                        match roster {
                            // request_response dials the provider itself. One
                            // already reported is still connected: a refresh
                            // re-asks it as a notice peer, and an old build
                            // has nothing new to say.
                            Some((title_id, ask)) => {
                                let fresh: Vec<PeerId> = others.filter(|peer| !self.roster_seen.contains(peer)).collect();
                                for peer in unasked(&mut self.roster_asked, fresh) {
                                    self.send_roster_ask(peer, title_id, &ask);
                                }
                            }
                            // Hand-off on identify does the rest.
                            None => {
                                let fresh = self.seed_searches.get_mut(&id).map(|(_, dialled)| unasked(dialled, others));
                                for peer in fresh.unwrap_or_default() {
                                    if !self.connected.contains(&peer) {
                                        let _ = self.swarm.dial(peer);
                                    }
                                }
                            }
                        }
                    }
                    if last {
                        tracing::debug!(query = ?id, roster = self.roster_searches.contains_key(&id), "panel provider lookup finished");
                        self.seed_searches.remove(&id);
                        self.roster_searches.remove(&id);
                    }
                    return;
                }
                if let Some(&peer) = self.peer_lookups.get(&id) {
                    if let Ok(kad::GetProvidersOk::FoundProviders { providers, .. }) = &res {
                        if providers.contains(&peer) {
                            // Many DHT servers return the same provider. Once our
                            // target is found, further walking and repeated dials
                            // waste bandwidth while it is offline or behind a NAT.
                            self.peer_lookups.remove(&id);
                            if let Some(mut query) = self.swarm.behaviour_mut().kad.query_mut(&id) { query.finish(); }
                            if !self.connected.contains(&peer) {
                                tracing::info!(%peer, "friend found in DHT; dialling once and finishing lookup");
                                let _ = self.swarm.dial(peer);
                            }
                        }
                    }
                    if last {
                        self.peer_lookups.remove(&id);
                    }
                    return;
                }
                // One result type serves two registries -- the §10 session
                // directory and the §17.3 shard registry. Dispatch on which
                // query this belongs to; matching only one would silently
                // swallow the other's providers.
                let is_session = self
                    .pending_session_search
                    .as_ref()
                    .is_some_and(|(qid, _)| *qid == id);
                let is_shard = self
                    .pending_shard_search
                    .as_ref()
                    .is_some_and(|(qid, _)| *qid == id);
                let is_relay = self.pending_relay_search == Some(id);
                let local = *self.swarm.local_peer_id();

                if is_session {
                    let Some((search_id, mut asked)) = self.pending_session_search.take() else {
                        return;
                    };
                    tracing::info!(query = ?id, last, result = ?res, "session provider lookup progress");
                    if let Ok(kad::GetProvidersOk::FoundProviders { providers, .. }) = res {
                        for peer in providers {
                            if peer == local || asked.contains(&peer) {
                                continue;
                            }
                            asked.push(peer);
                            self.swarm
                                .behaviour_mut()
                                .session
                                .send_request(&peer, SessionQuery { schema: 1 });
                        }
                    }
                    if !last {
                        self.pending_session_search = Some((search_id, asked));
                    }
                } else if is_shard {
                    let Some((search_id, mut asked)) = self.pending_shard_search.take() else {
                        return;
                    };
                    if let Ok(kad::GetProvidersOk::FoundProviders { providers, .. }) = res {
                        for peer in providers {
                            if peer == local || asked.contains(&peer) {
                                continue;
                            }
                            asked.push(peer);
                            let title_id = self.title_id;
                            let req_id = self
                                .swarm
                                .behaviour_mut()
                                .shard
                                .send_request(&peer, ShardQuery { schema: 1, title_id });
                            self.pending_shard_queries.insert(req_id, peer);
                        }
                    }
                    if !last {
                        self.pending_shard_search = Some((search_id, asked));
                    } else {
                        // Sweep finished. If nobody advertised a shard there
                        // are no descriptor queries outstanding, so this
                        // settles into creating one immediately -- the first
                        // node in should not wait a full rescan interval.
                        self.shard_swept_at = Some(Instant::now());
                        self.shard_settle();
                    }
                } else if is_relay {
                    // Relays found in the DHT (§9). Dial each one we do not
                    // already reserve through; the identify handler then holds a
                    // circuit reservation on any that offer the hop on a routable
                    // address. Dialing (not reserving here) reuses that one path.
                    if let Ok(kad::GetProvidersOk::FoundProviders { providers, .. }) = res {
                        for peer in providers {
                            if peer == local || self.auto_relays.contains(&peer) {
                                continue;
                            }
                            if self.auto_relays.len() >= MAX_AUTO_RELAYS {
                                break;
                            }
                            self.wanted_peers.insert(peer);
                            if let Err(err) = self.swarm.dial(peer) {
                                tracing::debug!(%peer, %err, "dial of discovered relay failed");
                            }
                        }
                    }
                    if last {
                        self.pending_relay_search = None;
                    }
                }
            }
            kad::QueryResult::StartProviding(res) => {
                if let Some(team) = self.club_key_queries.remove(&id) {
                    if res.is_ok() && successes > 0 {
                        self.club_key_at.insert(team, Instant::now());
                    }
                }
                tracing::info!(ok = res.is_ok(), stored = successes, "provider record published");
                if self.own_key_query == Some(id) {
                    self.own_key_query = None;
                    // kad reports Ok even when no DHT node took the record.
                    if res.is_ok() && successes > 0 {
                        self.own_key_at = Some(Instant::now());
                    }
                }
            }
            kad::QueryResult::Bootstrap(res) => {
                if last {
                    tracing::info!(ok = res.is_ok(), "kad bootstrap finished");
                }
            }
            _ => {}
        }
    }
}

impl Engine {
    fn lock_notices(&self) -> std::sync::MutexGuard<'_, crate::notice::Slots> {
        self.notices.lock().unwrap_or_else(|error| error.into_inner())
    }

    /// Ask `peer` who it is. Callers go through [`unasked`], so each peer is
    /// asked once per `Command::Roster` (a scan or a refresh).
    fn send_roster_ask(&mut self, peer: PeerId, title_id: u32, ask: &crate::notice::Signed) {
        tracing::debug!(%peer, "roster ask");
        let id = self.swarm.behaviour_mut().notice.send_request(&peer, NoticeRequest::Ask(ask.clone()));
        self.roster_asks.insert(id, title_id);
    }

    /// Look up who provides `title_id`'s shard key: a roster search that asks
    /// each provider with `ask`, or (None) a seed that dials each one. It
    /// starts at once: the routing table holds the bootstrap nodes from spawn,
    /// and waiting for kad's bootstrap to report cost 17 s
    /// (docs/technical/rexnet-notices.md).
    fn title_lookup(&mut self, title_id: u32, ask: Option<crate::notice::Signed>) {
        // Roster scans come every few seconds and an Amino lookup takes
        // longer, so a title has one roster lookup and one seed running at a time.
        let running = match ask {
            Some(_) => lookup_running(&self.roster_searches, title_id),
            None => lookup_running(&self.seed_searches, title_id),
        };
        if running {
            tracing::debug!(title = %format!("{title_id:08X}"), roster = ask.is_some(), "lookup still running; not starting another");
            return;
        }
        tracing::debug!(title = %format!("{title_id:08X}"), roster = ask.is_some(), "panel provider lookup");
        let key = kad::RecordKey::new(&crate::protocol::shard_key(title_id));
        let id = self.swarm.behaviour_mut().kad.get_providers(key);
        if let Some(ask) = ask {
            self.roster_searches.insert(id, (title_id, ask));
        } else {
            self.seed_searches.insert(id, (title_id, HashSet::new()));
        }
    }

    /// Announce that we provide `title_id`'s shard key, with the addresses we
    /// have now. Amino servers drop an announcement that carries none, so a
    /// beacon is announced again on every newly confirmed external address (a
    /// relay circuit included) and every BEACON_REPUBLISH.
    fn provide_beacon(&mut self, title_id: u32) {
        let key = kad::RecordKey::new(&crate::protocol::shard_key(title_id));
        if let Err(err) = self.swarm.behaviour_mut().kad.start_providing(key) {
            tracing::warn!(title = %format!("{title_id:08X}"), %err, "beacon provider record publish failed");
        }
    }

    fn provide_beacons(&mut self) {
        for title_id in self.beacons.clone() {
            self.provide_beacon(title_id);
        }
    }

    /// Only `targeted` sends are tracked: their Ack is a delivery, while a
    /// hand-off's or forward's Ack must not clear or report a targeted notice.
    fn send_notice(&mut self, peer: PeerId, signed: crate::notice::Signed, targeted: bool) {
        let id = self.swarm.behaviour_mut().notice.send_request(&peer, NoticeRequest::Notice(signed));
        if targeted {
            self.notice_sends.insert(id, peer);
        }
    }

    /// Verify and keep a notice. A newer broadcast goes on to every notice
    /// peer but the one it came from; that is the only forwarding there is.
    fn receive_notice(&mut self, from: PeerId, signed: crate::notice::Signed) {
        let notice = match crate::notice::open_notice(&self.notice_key, &signed, crate::mailbox::unix_now()) {
            Ok(notice) => notice,
            Err(reason) => {
                tracing::debug!(%from, ?reason, "notice refused");
                return;
            }
        };
        let seq = notice.seq;
        let local = *self.swarm.local_peer_id();
        // Our own PublishNotice skips the title check, so the panel holds and
        // hands off what it publishes.
        let title = if from == local { None } else { Some(self.title_id) };
        let adopted = self.lock_notices().offer(notice, &signed, &local, title, crate::mailbox::unix_now());
        if adopted.is_some() {
            tracing::info!(%from, seq, ?adopted, "notice received");
        } else {
            tracing::debug!(%from, seq, "notice received");
        }
        if adopted == Some(crate::notice::Slot::Broadcast) {
            let peers: Vec<PeerId> = self.notice_peers.iter().copied().filter(|peer| *peer != from).collect();
            for peer in peers {
                self.send_notice(peer, signed.clone(), false);
            }
        }
    }

    /// `peer` identified as speaking the notice protocol: give it our
    /// broadcast notice and any targeted one waiting for it.
    fn notice_peer_identified(&mut self, peer: PeerId) {
        if !self.notice_peers.insert(peer) {
            return;
        }
        let handoff = self.lock_notices().handoff(crate::mailbox::unix_now());
        if let Some(signed) = handoff {
            self.send_notice(peer, signed, false);
        }
        if let Some(signed) = self.notice_targets.get(&peer).cloned() {
            self.send_notice(peer, signed, true);
        }
    }
}

/// Whether `searches` (by query: title, then the rest) has a lookup of
/// `title_id` running.
fn lookup_running<V>(searches: &HashMap<kad::QueryId, (u32, V)>, title_id: u32) -> bool {
    searches.values().any(|(title, _)| *title == title_id)
}

/// The peers of `peers` not yet in `seen`, in order, now recorded there. Every
/// roster ask and seed dial goes through this: the roster command and each kad
/// GetProviders step list the same providers, and an unreachable one costs a
/// dial per ask.
fn unasked(seen: &mut HashSet<PeerId>, peers: impl IntoIterator<Item = PeerId>) -> Vec<PeerId> {
    peers.into_iter().filter(|peer| seen.insert(*peer)).collect()
}

/// A roster ask that failed because the peer speaks no notice protocol: a
/// connected runner on a build from before notices. Any other failure says
/// nothing about its build.
fn old_build_event(error: &request_response::OutboundFailure, peer: PeerId, title_id: u32) -> Option<Event> {
    matches!(error, request_response::OutboundFailure::UnsupportedProtocols)
        .then_some(Event::RosterOldBuild { peer, title_id })
}

/// Where a roster answer came from, for the panel to dial next time: the
/// newest link we dialled, ending in `/p2p/<peer>`.
fn roster_addr(links: &[Link], peer: PeerId) -> Option<String> {
    links.iter().rev().find_map(|link| link.remote.clone()?.with_p2p(peer).ok()).map(|addr| addr.to_string())
}

fn hex16(bytes: &[u8; 16]) -> String {
    bytes.iter().map(|b| format!("{b:02x}")).collect()
}

#[cfg(test)]
mod session_data_tests {
    use super::*;

    #[test]
    fn recovery_routes_are_bounded_expire_and_offer_a_different_path_after_quic_failure() {
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let relay = Keypair::generate_ed25519().public().to_peer_id();
        let now = Instant::now();
        let quic: Multiaddr = "/ip4/1.2.3.4/udp/1234/quic-v1".parse().unwrap();
        let tcp: Multiaddr = "/ip4/1.2.3.4/tcp/2345".parse().unwrap();
        let circuit: Multiaddr = format!("/ip4/5.6.7.8/udp/4001/quic-v1/p2p/{relay}/p2p-circuit/p2p/{peer}").parse().unwrap();
        let mut routes = HashMap::new();
        remember_routes(&mut routes, peer, [quic.clone(), tcp.clone(), circuit.clone(), tcp.clone()], now);
        assert_eq!(reconnect_routes(routes.get(&peer), now), vec![tcp.clone(), circuit.clone(), quic.clone()]);
        routes.get_mut(&peer).unwrap().avoid_quic_until = Some(now + SHARD_RETRY_MAX);
        assert_eq!(reconnect_routes(routes.get(&peer), now), vec![tcp.clone(), circuit]);
        assert_eq!(reconnect_routes(routes.get(&peer), now + SHARD_RETRY_MAX).len(), 3);
        assert!(reconnect_routes(routes.get(&peer), now + ROUTE_CACHE_TTL).is_empty());
        let mut only_quic = HashMap::new();
        remember_routes(&mut only_quic, peer, [quic.clone()], now);
        only_quic.get_mut(&peer).unwrap().avoid_quic_until = Some(now + SHARD_RETRY_MAX);
        assert_eq!(reconnect_routes(only_quic.get(&peer), now), vec![quic]);
        for port in 1..=100 {
            remember_routes(&mut routes, peer, [format!("/ip4/1.2.3.4/tcp/{port}").parse().unwrap()], now);
        }
        assert_eq!(routes[&peer].addresses.len(), ROUTE_CACHE_ADDRS);
        assert!(routes[&peer].addresses.iter().any(direct_quic));
        assert!(routes[&peer].addresses.iter().any(|a| circuit_relay(a).is_some()));
        for _ in 0..ROUTE_CACHE_PEERS + 1 {
            remember_routes(&mut routes, Keypair::generate_ed25519().public().to_peer_id(), [tcp.clone()], now);
        }
        assert_eq!(routes.len(), ROUTE_CACHE_PEERS);
        remember_routes(&mut routes, peer, [tcp], now + ROUTE_CACHE_TTL);
        assert_eq!(routes.len(), 1);
    }

    #[test]
    fn recovery_does_not_cache_unsupported_addresses_or_a_tcp_source_port() {
        use libp2p::core::{ConnectedPoint, Endpoint, transport::PortUse};
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let other = Keypair::generate_ed25519().public().to_peer_id();
        let mut routes = HashMap::new();
        for addr in ["/ip4/0.0.0.0/tcp/1234".to_string(), "/ip6/::/udp/1234/quic-v1".into(),
            "/ip4/1.2.3.4/tcp/0".into(), "/ip4/1.2.3.4/udp/0/quic-v1".into(),
            "/ip4/1.2.3.4/tcp/1234/ws".into(), format!("/ip4/1.2.3.4/tcp/1234/p2p/{other}")] {
            remember_routes(&mut routes, peer, [addr.parse().unwrap()], Instant::now());
        }
        assert!(routes.is_empty());
        let tcp: Multiaddr = "/ip4/127.0.0.1/tcp/1234".parse().unwrap();
        let incoming = ConnectedPoint::Listener { local_addr: tcp.clone(), send_back_addr: tcp.clone() };
        assert!(reconnect_address(&incoming).is_none(), "a TCP source port cannot be redialled");
        let outgoing = ConnectedPoint::Dialer { address: tcp.clone(), role_override: Endpoint::Dialer, port_use: PortUse::Reuse };
        assert_eq!(reconnect_address(&outgoing), Some(tcp));
        let quic: Multiaddr = "/ip4/127.0.0.1/udp/1234/quic-v1".parse().unwrap();
        let incoming = ConnectedPoint::Listener { local_addr: quic.clone(), send_back_addr: quic.clone() };
        assert_eq!(reconnect_address(&incoming), Some(quic));
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn recovery_releases_a_game_peer_that_stays_offline() {
        let mut p = super::friend_tests::pair(["", ""]).await;
        let peer = p.b.peer_id;
        p.a.cmd_tx.send(Command::Punch { peer, reply: None }).unwrap();
        super::friend_tests::settle(&p.a, peer).await;
        // Open the real tunnel before shutting down the remote engine.
        wait_event(&mut p.a, |e| matches!(e, Event::Degraded { peer: id } if *id == peer)).await;
        p.b.cmd_tx.send(Command::Shutdown).unwrap();
        let started = Instant::now();
        tokio::time::sleep(Duration::from_millis(50)).await;
        let (reply, receive) = oneshot::channel();
        p.a.cmd_tx.send(Command::DropControlForTest { peer, reply }).unwrap();
        receive.await.unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::PeerDisconnected { peer: id } if *id == peer)).await;
        assert!(started.elapsed() >= PEER_RECOVERY_GRACE, "the grace period was bypassed");
        assert!(started.elapsed() < PEER_RECOVERY_GRACE + Duration::from_secs(2), "offline peer retained indefinitely");
        assert!(super::friend_tests::no_event(&mut p.a, Duration::from_millis(300),
            |e| matches!(e, Event::PeerDisconnected { peer: id } if *id == peer)).await,
            "departure must be reported once");
        p.a.cmd_tx.send(Command::Shutdown).unwrap();
        let root = p.root.clone();
        drop(p);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn recovery_switches_to_tcp_after_a_real_quic_packet_blackhole() {
        use std::sync::atomic::AtomicBool;
        let _network = super::friend_tests::network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-quic-recovery-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut host = spawn(Keypair::generate_ed25519(), super::friend_tests::config(&root, "host")).await.unwrap();
        let mut client = spawn(Keypair::generate_ed25519(), super::friend_tests::config(&root, "client")).await.unwrap();
        let (h, c) = (host.peer_id, client.peer_id);
        super::friend_tests::loopback_address(&host).await;
        let quic = tokio::time::timeout(Duration::from_secs(3), async {
            loop {
                let (reply, receive) = oneshot::channel();
                host.cmd_tx.send(Command::LocalAddrs { reply }).unwrap();
                if let Some(addr) = receive.await.unwrap().into_iter()
                    .find(|a| a.to_string().starts_with("/ip4/") && direct_quic(a)) { break addr; }
                tokio::time::sleep(Duration::from_millis(10)).await;
            }
        }).await.expect("host did not open an IPv4 QUIC listener");
        let port = quic.iter().find_map(|p| if let Protocol::Udp(port) = p { Some(port) } else { None }).unwrap();
        let target = SocketAddr::from(([127, 0, 0, 1], port));
        let proxy = UdpSocket::bind("127.0.0.1:0").await.unwrap();
        let proxy_port = proxy.local_addr().unwrap().port();
        let blocked = Arc::new(AtomicBool::new(false));
        let block = blocked.clone();
        let proxy_task = tokio::spawn(async move {
            let mut client_addr = None;
            let mut packet = vec![0; 65536];
            loop {
                let (len, from) = proxy.recv_from(&mut packet).await.unwrap();
                if block.load(Ordering::Relaxed) { continue; }
                let to = if from == target { client_addr } else { client_addr = Some(from); Some(target) };
                if let Some(to) = to { let _ = proxy.send_to(&packet[..len], to).await; }
            }
        });
        client.cmd_tx.send(Command::ConnectManual {
            multiaddr: format!("/ip4/127.0.0.1/udp/{proxy_port}/quic-v1/p2p/{h}"),
        }).unwrap();
        let Event::PeerConnected { virtual_ip: host_ip, .. } = wait_event(&mut client,
            |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == h)).await else { unreachable!() };
        client.cmd_tx.send(Command::Punch { peer: h, reply: None }).unwrap();
        super::friend_tests::settle(&client, h).await;
        let Event::PeerConnected { virtual_ip: client_ip, .. } = wait_event(&mut host,
            |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == c)).await else { unreachable!() };
        client.cmd_tx.send(Command::SendDatagram {
            virtual_ip: host_ip, src_port: 3074, dst_port: 3074, data: b"before-block".to_vec(), reliable: false,
        }).unwrap();
        wait_event(&mut host, |e| matches!(e, Event::Datagram { data, .. } if data == b"before-block")).await;
        let (reply, receive) = oneshot::channel();
        client.cmd_tx.send(Command::ControlPathsForTest { peer: h, reply }).unwrap();
        assert_eq!(receive.await.unwrap(), vec![true], "the fixture must start on QUIC alone");
        blocked.store(true, Ordering::Relaxed);
        let started = Instant::now();
        let mut sends = tokio::time::interval(Duration::from_millis(100));
        tokio::time::timeout(Duration::from_secs(25), async {
            loop {
                tokio::select! {
                    _ = sends.tick() => {
                        client.cmd_tx.send(Command::SendDatagram { virtual_ip: host_ip, src_port: 3074,
                            dst_port: 3074, data: b"after-block".to_vec(), reliable: false }).unwrap();
                    }
                    Some(event) = host.evt_rx.recv() => match event {
                        Event::PeerDisconnected { peer } if peer == c => panic!("host lost the game peer instead of switching transport"),
                        Event::Datagram { data, .. } if data == b"after-block" => break,
                        _ => {}
                    },
                    Some(event) = client.evt_rx.recv() => {
                        assert!(!matches!(event, Event::PeerDisconnected { peer } if peer == h), "client reported departure during recovery");
                    }
                }
            }
        }).await.expect("game traffic never recovered from a QUIC blackhole");
        assert!(started.elapsed() >= Duration::from_secs(8), "QUIC was bypassed before its transport timeout");
        let (reply, receive) = oneshot::channel();
        client.cmd_tx.send(Command::ControlPathsForTest { peer: h, reply }).unwrap();
        let paths = receive.await.unwrap();
        assert!(!paths.is_empty() && paths.iter().all(|quic| !quic), "the replacement must use TCP");
        host.cmd_tx.send(Command::SendDatagram { virtual_ip: client_ip, src_port: 3074,
            dst_port: 3074, data: b"tcp-reply".to_vec(), reliable: false }).unwrap();
        let Event::Datagram { virtual_ip, .. } = wait_event(&mut client,
            |e| matches!(e, Event::Datagram { data, .. } if data == b"tcp-reply")).await else { unreachable!() };
        assert_eq!(virtual_ip, host_ip);
        println!("QUIC blackhole: two-way TCP game traffic restored in {:?}", started.elapsed());
        proxy_task.abort();
        host.cmd_tx.send(Command::Shutdown).unwrap();
        client.cmd_tx.send(Command::Shutdown).unwrap();
        drop((host, client));
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn recovery_reopens_game_traffic_without_reporting_departure() {
        let mut p = super::friend_tests::pair(["", ""]).await;
        let (a, b) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::Punch { peer: b, reply: None }).unwrap();
        super::friend_tests::settle(&p.a, b).await;
        let Event::PeerConnected { virtual_ip: a_ip, .. } =
            wait_event(&mut p.b, |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == a)).await
            else { unreachable!() };
        let b_ip = VIP_NETWORK_BASE | (u32::from(subnet::RESERVED_SUBNET) << 8)
            | u32::from(subnet::assign(&[a, b])[&b]);
        p.a.cmd_tx.send(Command::SendDatagram {
            virtual_ip: b_ip, src_port: 3074, dst_port: 3074, data: b"before".to_vec(), reliable: false,
        }).unwrap();
        wait_event(&mut p.b, |e| matches!(e, Event::Datagram { data, .. } if data == b"before")).await;
        let (reply, receive) = oneshot::channel();
        p.a.cmd_tx.send(Command::DropControlForTest { peer: b, reply }).unwrap();
        receive.await.unwrap();
        assert!(super::friend_tests::no_event(&mut p.a, Duration::from_millis(400),
            |e| matches!(e, Event::PeerDisconnected { peer } if *peer == b)).await,
            "temporary control loss immediately removed the game peer");
        p.a.cmd_tx.send(Command::SendDatagram {
            virtual_ip: b_ip, src_port: 3074, dst_port: 3074, data: b"after".to_vec(), reliable: false,
        }).unwrap();
        wait_event(&mut p.b, |e| matches!(e, Event::Datagram { data, .. } if data == b"after")).await;
        p.b.cmd_tx.send(Command::SendDatagram {
            virtual_ip: a_ip, src_port: 3074, dst_port: 3074, data: b"reply".to_vec(), reliable: false,
        }).unwrap();
        let Event::Datagram { virtual_ip, .. } = wait_event(&mut p.a,
            |e| matches!(e, Event::Datagram { data, .. } if data == b"reply")).await else { unreachable!() };
        assert_eq!(virtual_ip, b_ip, "the recovered peer's virtual IP changed");
        super::friend_tests::finish(p).await;
    }

    async fn wait_event(handles: &mut EngineHandles, matches: impl Fn(&Event) -> bool) -> Event {
        tokio::time::timeout(Duration::from_secs(8), async {
            loop {
                let event = handles.evt_rx.recv().await.expect("engine stopped");
                if matches(&event) { return event; }
            }
        }).await.expect("expected network event timed out")
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn loopback_session_query_and_two_way_fallback_tunnel() {
        loopback_session_and_voice(true).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn loopback_session_and_voice_direct_udp() {
        loopback_session_and_voice(false).await;
    }

    async fn loopback_session_and_voice(force_tunnel: bool) {
        let _network = super::friend_tests::network_lock();
        let root =std::env::temp_dir().join(format!("rexnet-link-check-{}-{}", std::process::id(), rand::random::<u64>()));
        let config = |name: &str| EngineConfig {
            data_dir: root.join(name), title_id: 0x11223344, display_name: name.into(),
            bootstrap: vec![], listen_port: 0, game_port: 0, relays: vec![], force_tunnel,
            notice_key: [0; 32],
        };
        let mut host = spawn(Keypair::generate_ed25519(), config("host")).await.unwrap();
        let mut client = spawn(Keypair::generate_ed25519(), config("client")).await.unwrap();
        let address = tokio::time::timeout(Duration::from_secs(3), async {
            loop {
                let (reply, receive) = oneshot::channel();
                host.cmd_tx.send(Command::LocalAddrs { reply }).unwrap();
                if let Some(address) = receive.await.unwrap().into_iter().find(|a| {
                    a.to_string().starts_with("/ip4/127.0.0.1/tcp/")
                }) { break address; }
                tokio::time::sleep(Duration::from_millis(10)).await;
            }
        }).await.unwrap();
        client.cmd_tx.send(Command::ConnectManual {
            multiaddr: format!("{address}/p2p/{}", host.peer_id),
        }).unwrap();
        let host_id = host.peer_id;
        let client_id = client.peer_id;
        let Event::PeerConnected { virtual_ip: host_ip, .. } = wait_event(&mut client,
            |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == host_id)).await else { unreachable!() };
        // The echo round trip also ensures the host processed its queued session update.
        host.cmd_tx.send(Command::SessionCreate {
            id: [7; 16], slots_total: 8, slots_open: 7, is_public: true,
        }).unwrap();
        host.cmd_tx.send(Command::SessionUpdate { slots_total: 8, slots_open: 6, data: vec![9] }).unwrap();
        tokio::time::timeout(Duration::from_secs(3), async {
            loop {
                let (reply, receive) = oneshot::channel();
                host.cmd_tx.send(Command::Echo { peer: client_id, reply }).unwrap();
                if matches!(receive.await, Ok(Ok(_))) { break; }
                // The dialler can publish PeerConnected before the listener
                // has polled its ConnectionEstablished event.
                tokio::time::sleep(Duration::from_millis(10)).await;
            }
        }).await.expect("host never processed its client connection");
        // Nothing in this exchange makes the client app-relevant to the host;
        // without this the host only announced it if the client happened to
        // become its circuit relay over a LAN interface (timing-dependent).
        host.cmd_tx.send(Command::ConnectPeer { peer: client_id }).unwrap();
        let Event::PeerConnected { virtual_ip: client_ip, .. } = wait_event(&mut host,
            |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == client_id)).await else { unreachable!() };
        // Repeated searches must still obtain the descriptor while provider
        // discovery is in flight. Known peers can answer before shard placement.
        for _ in 0..3 {
            client.cmd_tx.send(Command::SessionSearch).unwrap();
            tokio::time::sleep(Duration::from_millis(25)).await;
        }
        let Event::SessionFound { data, slots_open, .. } = wait_event(&mut client,
            |e| matches!(e, Event::SessionFound { host, .. } if *host == host_id)).await else { unreachable!() };
        assert_eq!((data, slots_open), (vec![9], 6));
        client.cmd_tx.send(Command::Punch { peer: host_id, reply: None }).unwrap();
        if !force_tunnel {
            let Event::PunchResult { ok, .. } = wait_event(&mut client,
                |e| matches!(e, Event::PunchResult { peer, .. } if *peer == host_id)).await else { unreachable!() };
            assert!(ok, "direct UDP must punch successfully; fallback is tested separately");
        }
        client.cmd_tx.send(Command::SendDatagram {
            virtual_ip: host_ip, src_port: 3074, dst_port: 3075, data: b"client-to-host".to_vec(), reliable: false,
        }).unwrap();
        let Event::Datagram { data, .. } = wait_event(&mut host,
            |e| matches!(e, Event::Datagram { dst_port: 3075, .. })).await else { unreachable!() };
        assert_eq!(data, b"client-to-host");
        host.cmd_tx.send(Command::SendDatagram {
            virtual_ip: client_ip, src_port: 3075, dst_port: 3074, data: b"host-to-client".to_vec(), reliable: false,
        }).unwrap();
        let Event::Datagram { data, .. } = wait_event(&mut client,
            |e| matches!(e, Event::Datagram { dst_port: 3074, .. })).await else { unreachable!() };
        assert_eq!(data, b"host-to-client");
        // Authoritative sender/receiver counts cover actual delivery on both
        // direct UDP and fallback streams, without counting queued frames twice.
        let stage = if force_tunnel {"tunnel_written"} else {"udp_socket_sent"};
        assert!(client.diagnostics.test_totals()["stages"][stage]["bytes"].as_u64().unwrap() > 0);
        let received = if force_tunnel {"tunnel_received"} else {"udp_received"};
        assert_eq!(host.diagnostics.test_totals()["stages"][received]["bytes"],b"client-to-host".len());
        assert!(client.diagnostics.test_bandwidth()["transport_streams"].as_array().unwrap()
            .iter().any(|r|r["total_bytes"].as_u64().unwrap_or(0)>0));
        // NetSocket_SendVdpTo keeps its u16 prefix and guest port 1001.
        // This fixture proves framing preservation, not speech-codec fidelity.
        let voice = vec![4, 0, 0x21, 0x43, 0x65, 0x87];
        client.cmd_tx.send(Command::SendDatagram {
            virtual_ip: host_ip, src_port: 1001, dst_port: 1001, data: voice.clone(), reliable: false,
        }).unwrap();
        let Event::Datagram { src_port, data, .. } = wait_event(&mut host,
            |e| matches!(e, Event::Datagram { dst_port: 1001, .. })).await else { unreachable!() };
        assert_eq!(src_port, 1001); assert_eq!(data, voice);
        let mut previous_session_frame = None;
        if !force_tunnel {
            // Keep an unused high-counter frame. Its rejection after reconnect
            // must come from new keys, rather than an already-seen counter.
            for _ in 0..100 {
                previous_session_frame = client.plane.seal(host_id, 3074, 3074, b"previous-session");
            }
            assert!(previous_session_frame.is_some());
        }
        host.cmd_tx.send(Command::SendDatagram {
            virtual_ip: client_ip, src_port: 1001, dst_port: 1001, data: voice.clone(), reliable: false,
        }).unwrap();
        let Event::Datagram { src_port, data, .. } = wait_event(&mut client,
            |e| matches!(e, Event::Datagram { dst_port: 1001, .. })).await else { unreachable!() };
        assert_eq!(src_port, 1001); assert_eq!(data, voice);
        // The UDP receive budget must include framing/authentication overhead,
        // not just the same 2048-byte guest payload supported by the tunnel.
        let large = vec![0x5A;tunnel::MAX_TUNNEL_PAYLOAD];
        if force_tunnel {
            client.cmd_tx.send(Command::SendDatagram { virtual_ip:host_ip,src_port:3074,
                dst_port:3074,data:large.clone(),reliable:false }).unwrap();
        } else { assert!(client.plane.try_send(host_ip,3074,3074,&large), "fixture must exercise direct UDP"); }
        wait_event(&mut host,|e|matches!(e,Event::Datagram{data,..} if data==&large)).await;
        client.cmd_tx.send(Command::SendDatagram { virtual_ip:host_ip,src_port:3074,
            dst_port:3074,data:b"after-large-frame".to_vec(),reliable:false }).unwrap();
        wait_event(&mut host,|e|matches!(e,Event::Datagram{data,..} if data==b"after-large-frame")).await;
        #[cfg(windows)]
        if !force_tunnel {
            // A failed probe to a departed peer produces an ICMP Port Unreachable
            // on Windows. It must not kill the socket shared by every live peer.
            let closed = UdpSocket::bind("127.0.0.1:0").await.unwrap();
            let address = closed.local_addr().unwrap();
            drop(closed);
            let (reply, receive) = oneshot::channel();
            host.cmd_tx.send(Command::GameSocketForTest { reply }).unwrap();
            receive.await.unwrap().send_to(b"closed-probe", address).await.unwrap();
            tokio::time::sleep(Duration::from_millis(200)).await;
            assert!(client.plane.try_send(host_ip,3074,3074,b"after-closed-probe"));
            wait_event(&mut host,|e|matches!(e,Event::Datagram{data,..} if data==b"after-closed-probe")).await;
        }
        let (reply, receive) = oneshot::channel();
        client.cmd_tx.send(Command::DropControlForTest { peer: host_id, reply }).unwrap();
        receive.await.unwrap();
        assert!(super::friend_tests::no_event(&mut client, Duration::from_millis(400),
            |e| matches!(e, Event::PeerDisconnected { peer } if *peer == host_id)).await);
        client.cmd_tx.send(Command::SendDatagram { virtual_ip: host_ip, src_port: 3074,
            dst_port: 3074, data: b"reconnected-client".to_vec(), reliable: false }).unwrap();
        wait_event(&mut host, |e| matches!(e, Event::Datagram { data, .. } if data == b"reconnected-client")).await;
        host.cmd_tx.send(Command::SendDatagram { virtual_ip: client_ip, src_port: 3074,
            dst_port: 3074, data: b"reconnected-host".to_vec(), reliable: false }).unwrap();
        wait_event(&mut client, |e| matches!(e, Event::Datagram { data, .. } if data == b"reconnected-host")).await;
        if let Some(old_frame) = previous_session_frame {
            tokio::time::timeout(Duration::from_secs(3), async {
                while !host.plane.has_keys(&client_id) || !client.plane.has_keys(&host_id) {
                    tokio::time::sleep(Duration::from_millis(10)).await;
                }
            }).await.expect("reconnected UDP peers did not negotiate fresh keys");
            assert!(host.plane.open(client_id, &old_frame).is_none(), "old-session ciphertext accepted after reconnect");
        }
        host.cmd_tx.send(Command::Shutdown).unwrap();
        client.cmd_tx.send(Command::Shutdown).unwrap();
        drop((host, client));
        tokio::time::sleep(Duration::from_millis(50)).await;
        if root.exists() { std::fs::remove_dir_all(root).unwrap(); }
    }

    #[test]
    fn game_relay_limits_allow_a_race_but_remain_bounded() {
        let peers = RelayPeers::default();
        let mut config = game_relay_config(peers.clone());
        assert_eq!(config.max_circuit_duration, Duration::from_secs(7200));
        assert_eq!(config.max_circuit_bytes, 256 * 1024 * 1024);
        let upstream = relay::Config::default();
        assert_eq!(config.max_circuits, upstream.max_circuits);
        assert_eq!(config.max_circuits_per_peer, upstream.max_circuits_per_peer);
        assert_eq!(config.max_reservations, upstream.max_reservations);
        assert_eq!(config.circuit_src_rate_limiters.len(), upstream.circuit_src_rate_limiters.len() + 1);
        assert_eq!(config.reservation_rate_limiters.len(), upstream.reservation_rate_limiters.len() + 1);
        // Game-sized circuits are for other rexnet nodes, not the public DHT.
        let player = Keypair::generate_ed25519().public().to_peer_id();
        let stranger = Keypair::generate_ed25519().public().to_peer_id();
        peers.lock().unwrap().insert(player);
        let addr: libp2p::Multiaddr = "/ip4/127.0.0.1/tcp/1".parse().unwrap();
        let now = Instant::now();
        for limiters in [&mut config.reservation_rate_limiters, &mut config.circuit_src_rate_limiters] {
            let only_rexnet = limiters.last_mut().unwrap();
            assert!(only_rexnet.try_next(player, &addr, now));
            assert!(!only_rexnet.try_next(stranger, &addr, now));
        }
        peers.lock().unwrap().remove(&player);
        assert!(!config.reservation_rate_limiters.last_mut().unwrap().try_next(player, &addr, now));
    }

    #[test]
    fn failed_shard_dials_back_off_to_a_bounded_interval() {
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let other = Keypair::generate_ed25519().public().to_peer_id();
        let now = Instant::now();
        let mut retries = HashMap::new();
        let mut due = |peer, secs| shard_retry_due(&mut retries, peer, now + Duration::from_secs(secs),
            LINK_MAINTENANCE_INTERVAL, SHARD_RETRY_MAX);
        assert!(due(peer, 0));
        assert!(!due(peer, 0));
        assert!(due(other, 0));
        // 5 s, then doubling: 10, 20, 40, and never more than a minute.
        for (not_yet, at) in [(4, 5), (14, 15), (34, 35), (74, 75), (134, 135), (194, 195)] {
            assert!(!due(peer, not_yet), "{not_yet}");
            assert!(due(peer, at), "{at}");
        }
        retries.remove(&peer); // a real disconnect permits immediate recovery
        assert!(shard_retry_due(&mut retries, peer, now + Duration::from_secs(195),
            LINK_MAINTENANCE_INTERVAL, SHARD_RETRY_MAX));
    }

    #[test]
    fn large_shards_resolve_each_member_rarely() {
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let now = Instant::now();
        let mut retries = HashMap::new();
        let mut due = |secs| shard_retry_due(&mut retries, peer, now + Duration::from_secs(secs),
            SHARD_RESOLVE_INTERVAL, SHARD_RESOLVE_INTERVAL);
        assert!(due(0));
        assert!(!due(PRESENCE_HEARTBEAT.as_secs())); // the next beat does not walk the DHT again
        assert!(!due(599));
        assert!(due(600));
        assert!(!due(1199));
    }

    #[test]
    fn silent_disconnected_members_have_left_the_shard() {
        let [quiet, gone, linked] = [(); 3].map(|_| Keypair::generate_ed25519().public().to_peer_id());
        let now = Instant::now();
        let long_ago = now - SHARD_MEMBER_TIMEOUT - Duration::from_secs(1);
        let heard = HashMap::from([(quiet, now - Duration::from_secs(60)), (gone, long_ago), (linked, long_ago)]);
        let connected = HashSet::from([linked]);
        assert_eq!(departed_members(&heard, &connected, now), vec![gone]);
    }

    #[test]
    fn only_an_unknown_address_sends_a_shard_dial_to_the_dht() {
        use libp2p::swarm::{dial_opts::PeerCondition, DialError};
        assert!(shard_dial_needs_dht(&DialError::NoAddresses));
        // A dial already in flight will report its own outcome.
        assert!(!shard_dial_needs_dht(&DialError::DialPeerConditionFalse(PeerCondition::DisconnectedAndNotDialing)));
    }

    #[test]
    fn relayed_links_renew_at_half_budget_and_yield_to_newer_or_direct_links() {
        let relay = Keypair::generate_ed25519().public().to_peer_id();
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let circuit: Multiaddr = format!("/ip4/188.40.66.89/udp/4001/quic-v1/p2p/{relay}/p2p-circuit/p2p/{peer}").parse().unwrap();
        assert_eq!(circuit_relay(&circuit), Some(relay));
        assert_eq!(circuit_relay(&format!("/ip4/1.2.3.4/tcp/1/p2p/{peer}").parse().unwrap()), None);

        let now = Instant::now();
        let link = |id, relayed, secs: u64, bytes| Link {
            id: ConnectionId::new_unchecked(id),
            relayed,
            quic: false,
            renew_at: relayed.then(|| now + Duration::from_secs(secs)),
            renew_bytes: relayed.then_some(bytes),
            remote: None,
            inbound: false,
        };
        let old = link(1, true, 60, 64 * 1024);
        assert!(!renewal_due(std::slice::from_ref(&old), now, 0));
        assert!(renewal_due(std::slice::from_ref(&old), now + Duration::from_secs(60), 0));
        assert!(renewal_due(std::slice::from_ref(&old), now, 64 * 1024)); // data runs out first in a race
        let fresh = link(2, true, 120, 128 * 1024);
        let links = [old, fresh];
        assert!(!renewal_due(&links, now + Duration::from_secs(60), 0)); // the replacement carries us
        assert_eq!(superseded(&links), vec![ConnectionId::new_unchecked(1)]);
        let links = [link(1, true, 60, 0), link(2, false, 0, 0), link(3, true, 60, 0)];
        assert!(!renewal_due(&links, now + Duration::from_secs(999), u64::MAX)); // direct never ages
        assert_eq!(superseded(&links), vec![ConnectionId::new_unchecked(1), ConnectionId::new_unchecked(3)]);
        assert!(superseded(&[link(4, true, 60, 0)]).is_empty());
        let mut links = [link(8, false, 0, 0), link(9, true, 60, 0), link(10, false, 0, 0)];
        links[0].quic = true;
        links[1].quic = true; // QUIC to a relay is a different route, not the failed direct path.
        let tcp: Multiaddr = "/ip4/1.2.3.4/tcp/1234".parse().unwrap();
        let quic: Multiaddr = "/ip4/1.2.3.4/udp/1234/quic-v1".parse().unwrap();
        assert_eq!(quic_links_replaced(&links, &tcp, true), vec![ConnectionId::new_unchecked(8)]);
        assert_eq!(quic_links_replaced(&links, &circuit, true), vec![ConnectionId::new_unchecked(8)]);
        assert!(quic_links_replaced(&links, &quic, true).is_empty());
        assert!(quic_links_replaced(&links, &tcp, false).is_empty());
        assert!(quic_links_replaced(&links[1..], &tcp, true).is_empty(), "normal relay renewal still retires superseded relays");
    }

    #[test]
    fn retired_tunnel_writer_cannot_close_its_replacement() {
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let (old, _) = mpsc::channel::<(Instant, Vec<u8>)>(1);
        let (replacement, _) = mpsc::channel::<(Instant, Vec<u8>)>(1);
        let mut writers = HashMap::from([(peer, (1, old))]);
        assert!(current_tunnel_writer(&writers, &peer, 1));
        writers.insert(peer, (2, replacement));
        assert!(!current_tunnel_writer(&writers, &peer, 1));
        assert!(current_tunnel_writer(&writers, &peer, 2));
        writers.remove(&peer);
        assert!(!current_tunnel_writer(&writers, &peer, 2));
    }

    #[test]
    fn served_descriptor_carries_the_latest_session_data() {
        let mut session = LocalSession {
            id: [7; 16],
            slots_total: 8,
            slots_open: 7,
            is_public: true,
            data: vec![1, 2, 3],
        };
        let desc = session_desc(&session, 0x5454_0079);
        assert_eq!(desc.data, vec![1, 2, 3]);
        assert!(!desc.requires_invite);
        // An update replaces what the next querier is served.
        session.data = vec![9];
        session.slots_open = 5;
        let desc = session_desc(&session, 0x5454_0079);
        assert_eq!((desc.data, desc.slots_open), (vec![9], 5));
    }
}

#[cfg(test)]
mod friend_tests {
    use super::*;
    use std::path::Path;

    /// Tests that start real engines share the LAN: mDNS feeds every engine
    /// in the process into every other's DHT, and those dials and connection
    /// limits make concurrent network tests flaky. They take turns.
    static NETWORK_TESTS: std::sync::Mutex<()> = std::sync::Mutex::new(());

    pub(super) fn network_lock() -> std::sync::MutexGuard<'static, ()> {
        NETWORK_TESTS.lock().unwrap_or_else(|e| e.into_inner())
    }

    pub(super) async fn wait_event(handles: &mut EngineHandles, matches: impl Fn(&Event) -> bool) -> Event {
        tokio::time::timeout(Duration::from_secs(8), async {
            loop {
                let event = handles.evt_rx.recv().await.expect("engine stopped");
                if matches(&event) {
                    return event;
                }
            }
        })
        .await
        .expect("expected network event timed out")
    }

    /// True when nothing matching arrives within `wait`.
    pub(super) async fn no_event(handles: &mut EngineHandles, wait: Duration, matches: impl Fn(&Event) -> bool) -> bool {
        tokio::time::timeout(wait, async {
            loop {
                let event = handles.evt_rx.recv().await.expect("engine stopped");
                if matches(&event) {
                    return;
                }
            }
        })
        .await
        .is_err()
    }

    pub(super) fn config(root: &Path, name: &str) -> EngineConfig {
        EngineConfig {
            data_dir: root.join(name),
            title_id: 0x11223344,
            display_name: name.into(),
            bootstrap: vec![],
            listen_port: 0,
            game_port: 0,
            relays: vec![],
            force_tunnel: true,
            notice_key: crate::notice::test_public(),
        }
    }

    pub(super) async fn loopback_address(handles: &EngineHandles) -> Multiaddr {
        tokio::time::timeout(Duration::from_secs(3), async {
            loop {
                let (reply, receive) = oneshot::channel();
                handles.cmd_tx.send(Command::LocalAddrs { reply }).unwrap();
                if let Some(address) =
                    receive.await.unwrap().into_iter().find(|a| a.to_string().starts_with("/ip4/127.0.0.1/tcp/"))
                {
                    break address;
                }
                tokio::time::sleep(Duration::from_millis(10)).await;
            }
        })
        .await
        .unwrap()
    }

    /// Round trip an echo so `handles` has processed every command sent before.
    pub(super) async fn settle(handles: &EngineHandles, peer: PeerId) {
        let (reply, receive) = oneshot::channel();
        handles.cmd_tx.send(Command::Echo { peer, reply }).unwrap();
        tokio::time::timeout(Duration::from_secs(3), receive).await.unwrap().unwrap().unwrap();
    }

    /// A plain libp2p node over TCP, Noise and Yamux, running only `behaviour`.
    pub(super) fn bare_swarm<B: NetworkBehaviour>(behaviour: impl FnOnce(&Keypair) -> B) -> Swarm<B> {
        libp2p::SwarmBuilder::with_new_identity()
            .with_tokio()
            .with_tcp(libp2p::tcp::Config::default(), libp2p::noise::Config::new, libp2p::yamux::Config::default)
            .unwrap()
            .with_behaviour(behaviour)
            .unwrap()
            .with_swarm_config(|c| c.with_idle_connection_timeout(Duration::from_secs(60)))
            .build()
    }

    /// A public DHT peer, not a RexNet node: identify and nothing else.
    fn public_peer() -> Swarm<identify::Behaviour> {
        bare_swarm(|key| identify::Behaviour::new(identify::Config::new("/ipfs/0.1.0".into(), key.public())))
    }

    #[tokio::test]
    async fn protobuf_sizes_cover_real_dht_and_identify_exchange() {
        #[derive(NetworkBehaviour)]
        struct MetadataPeer { kad:kad::Behaviour<MemoryStore>, identify:identify::Behaviour }
        crate::diagnostics::install_protocol_measurement();
        let probe = crate::diagnostics::Recorder::start(&std::env::temp_dir(),0,"protocol-size-fixture");
        let before = probe.test_totals();
        let make = |key:&Keypair| MetadataPeer {
            kad:kad::Behaviour::new(key.public().to_peer_id(),MemoryStore::new(key.public().to_peer_id())),
            identify:identify::Behaviour::new(identify::Config::new("rexnet/size-fixture".into(),key.public())),
        };
        let mut server = bare_swarm(make);
        server.behaviour_mut().kad.set_mode(Some(kad::Mode::Server));
        let addr = bare_listen(&mut server).await;
        let mut client = bare_swarm(make);
        client.behaviour_mut().kad.set_mode(Some(kad::Mode::Client));
        client.behaviour_mut().kad.add_address(server.local_peer_id(),addr);
        let query = client.behaviour_mut().kad.get_providers(kad::RecordKey::new(&b"rexnet-size-fixture"));
        tokio::time::timeout(Duration::from_secs(5), async {
            let (mut identified,mut replied) = (false,false);
            while !identified || !replied {
                tokio::select! {
                    _ = server.select_next_some() => {},
                    event = client.select_next_some() => match event {
                        SwarmEvent::Behaviour(MetadataPeerEvent::Identify(identify::Event::Received { .. })) => identified=true,
                        SwarmEvent::Behaviour(MetadataPeerEvent::Kad(kad::Event::OutboundQueryProgressed {id,step,..}))
                            if id==query && step.last => replied=true,
                        _ => {},
                    }
                }
            }
        }).await.expect("real discovery exchange must finish");
        let after = probe.test_totals();
        for key in ["dht/get_providers/received/complete","dht/get_providers/encoded/complete",
            "identify/identify/received/complete"] {
            let first = before["protobuf_message_totals"][key]["events"].as_u64().unwrap_or(0);
            let last = after["protobuf_message_totals"][key]["events"].as_u64().unwrap_or(0);
            assert!(last>first,"shared codec must attribute actual traffic to {key}");
            assert!(after["protobuf_message_totals"][key]["max_payload_bytes"].as_u64().unwrap()>0);
        }
        if let Some(path) = std::env::var_os("XRECOMP_PROTOCOL_TEST_EVIDENCE") {
            std::fs::write(path,serde_json::to_vec_pretty(&probe.test_bandwidth()).unwrap()).unwrap();
        }
    }

    /// Listen on loopback TCP; the address ends in the node's /p2p/.
    pub(super) async fn bare_listen<B: NetworkBehaviour>(swarm: &mut Swarm<B>) -> Multiaddr {
        swarm.listen_on("/ip4/127.0.0.1/tcp/0".parse().unwrap()).unwrap();
        let peer = *swarm.local_peer_id();
        loop {
            if let SwarmEvent::NewListenAddr { address, .. } = swarm.select_next_some().await {
                return address.with(Protocol::P2p(peer));
            }
        }
    }

    /// Whether `swarm` produces an event `matches` accepts within `wait`.
    async fn swarm_event<B: NetworkBehaviour>(
        swarm: &mut Swarm<B>,
        wait: Duration,
        matches: impl Fn(&SwarmEvent<B::ToSwarm>) -> bool,
    ) -> bool {
        tokio::time::timeout(wait, async { while !matches(&swarm.select_next_some().await) {} }).await.is_ok()
    }

    pub(super) struct Pair {
        pub(super) root: PathBuf,
        pub(super) a: EngineHandles,
        pub(super) b: EngineHandles,
        _network: std::sync::MutexGuard<'static, ()>,
    }

    /// Engines `a` and `b` on loopback, `a` dialling `b`. Each side's
    /// friends.txt is seeded first, with `{peer}` replaced by the other side.
    pub(super) async fn pair(files: [&str; 2]) -> Pair {
        let network = network_lock();
        let root =std::env::temp_dir().join(format!("rexnet-friend-{}-{}", std::process::id(), rand::random::<u64>()));
        let (ka, kb) = (Keypair::generate_ed25519(), Keypair::generate_ed25519());
        let (pa, pb) = (ka.public().to_peer_id(), kb.public().to_peer_id());
        for (name, other, text) in [("a", pb, files[0]), ("b", pa, files[1])] {
            std::fs::create_dir_all(root.join(name)).unwrap();
            std::fs::write(root.join(name).join("friends.txt"), text.replace("{peer}", &other.to_base58())).unwrap();
        }
        let b = spawn(kb, config(&root, "b")).await.unwrap();
        let mut a = spawn(ka, config(&root, "a")).await.unwrap();
        let address = loopback_address(&b).await;
        a.cmd_tx.send(Command::ConnectManual { multiaddr: format!("{address}/p2p/{pb}") }).unwrap();
        wait_event(&mut a, |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == pb)).await;
        // b must see the connection too, or its first request to a dials a
        // and fails ("Failed to dial the requested peer").
        tokio::time::timeout(Duration::from_secs(5), async {
            loop {
                let (reply, receive) = oneshot::channel();
                b.cmd_tx.send(Command::Echo { peer: pa, reply }).unwrap();
                if matches!(receive.await, Ok(Ok(_))) {
                    break;
                }
                tokio::time::sleep(Duration::from_millis(20)).await;
            }
        })
        .await
        .expect("b never saw the connection");
        Pair { root, a, b, _network: network }
    }

    pub(super) async fn finish(pair: Pair) {
        pair.a.cmd_tx.send(Command::Shutdown).unwrap();
        pair.b.cmd_tx.send(Command::Shutdown).unwrap();
        let root = pair.root.clone();
        drop(pair);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_request_reports_the_targets_name_and_an_accept_makes_both_friends() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::FriendRequest { peer: pb, note: "race?".into() }).unwrap();
        let Event::FriendRequest { display_name, note, .. } =
            wait_event(&mut p.b, |e| matches!(e, Event::FriendRequest { peer, .. } if *peer == pa)).await
        else {
            unreachable!()
        };
        assert_eq!((display_name.as_str(), note.as_str()), ("a", "race?"));
        let Event::FriendRequestSent { display_name, .. } =
            wait_event(&mut p.a, |e| matches!(e, Event::FriendRequestSent { .. })).await
        else {
            unreachable!()
        };
        assert_eq!(display_name, "b");
        p.b.cmd_tx.send(Command::FriendAccept { peer: pa }).unwrap();
        let Event::FriendAccepted { display_name, .. } =
            wait_event(&mut p.a, |e| matches!(e, Event::FriendAccepted { peer, .. } if *peer == pb)).await
        else {
            unreachable!()
        };
        assert_eq!(display_name, "b");
        let a_file = std::fs::read_to_string(p.root.join("a/friends.txt")).unwrap();
        assert!(a_file.contains(&format!("{} friend b\t", pb.to_base58())), "{a_file}");
        let b_file = std::fs::read_to_string(p.root.join("b/friends.txt")).unwrap();
        assert!(b_file.contains(&format!("{} friend a\t", pa.to_base58())), "{b_file}");
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_blocked_peer_cannot_request_again() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::FriendRequest { peer: pb, note: String::new() }).unwrap();
        wait_event(&mut p.b, |e| matches!(e, Event::FriendRequest { .. })).await;
        p.b.cmd_tx.send(Command::FriendDecline { peer: pa, block: true }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::FriendRemoved { peer } if *peer == pb)).await;
        p.a.cmd_tx.send(Command::FriendRequest { peer: pb, note: "again".into() }).unwrap();
        // Delivered (a hears b's name back), but b drops it.
        wait_event(&mut p.a, |e| matches!(e, Event::FriendRequestSent { .. })).await;
        assert!(no_event(&mut p.b, Duration::from_millis(500), |e| matches!(e, Event::FriendRequest { .. })).await);
        let b_file = std::fs::read_to_string(p.root.join("b/friends.txt")).unwrap();
        assert!(b_file.contains(&format!("{} blocked", pa.to_base58())), "{b_file}");
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_saved_request_is_resent_when_the_peer_connects() {
        let mut p = pair(["{peer} out b\tsaved note\n", ""]).await;
        let Event::FriendRequest { note, .. } = wait_event(&mut p.b, |e| matches!(e, Event::FriendRequest { .. })).await
        else {
            unreachable!()
        };
        assert_eq!(note, "saved note");
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_peer_we_removed_while_it_was_away_is_told_on_its_next_presence() {
        // a still lists b; b forgot a while a was offline.
        let mut p = pair(["{peer} friend b\t\n", ""]).await;
        let pb = p.b.peer_id;
        wait_event(&mut p.a, |e| matches!(e, Event::FriendRemoved { peer } if *peer == pb)).await;
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn an_invite_carries_its_note_and_can_be_revoked() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::SessionCreate { id: [5; 16], slots_total: 4, slots_open: 3, is_public: false }).unwrap();
        p.a.cmd_tx.send(Command::SendInvite { peer: pb, note: "join me".into() }).unwrap();
        let Event::InviteReceived { session_id, note, .. } =
            wait_event(&mut p.b, |e| matches!(e, Event::InviteReceived { .. })).await
        else {
            unreachable!()
        };
        assert_eq!((session_id, note.as_str()), ([5; 16], "join me"));
        p.a.cmd_tx.send(Command::RevokeInvite { peer: pb }).unwrap();
        let Event::InviteRevoked { peer, session_id } =
            wait_event(&mut p.b, |e| matches!(e, Event::InviteRevoked { .. })).await
        else {
            unreachable!()
        };
        assert_eq!((peer, session_id), (pa, [5; 16]));
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_private_session_is_served_to_friends_but_not_strangers() {
        let mut p = pair(["{peer} friend b\t\n", "{peer} friend a\t\n"]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::SessionCreate { id: [6; 16], slots_total: 4, slots_open: 3, is_public: false }).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::SessionQuery { peer: pa }).unwrap();
        let Event::SessionFound { session_id, requires_invite, .. } =
            wait_event(&mut p.b, |e| matches!(e, Event::SessionFound { host, .. } if *host == pa)).await
        else {
            unreachable!()
        };
        assert_eq!((session_id, requires_invite), ([6; 16], true));
        // A stranger connected to a gets nothing.
        let mut c = spawn(Keypair::generate_ed25519(), config(&p.root, "c")).await.unwrap();
        let address = loopback_address(&p.a).await;
        c.cmd_tx.send(Command::ConnectManual { multiaddr: format!("{address}/p2p/{pa}") }).unwrap();
        wait_event(&mut c, |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == pa)).await;
        c.cmd_tx.send(Command::SessionQuery { peer: pa }).unwrap();
        assert!(no_event(&mut c, Duration::from_secs(1), |e| matches!(e, Event::SessionFound { .. })).await);
        c.cmd_tx.send(Command::Shutdown).unwrap();
        finish(p).await;
    }

    #[test]
    fn our_own_record_is_republished_until_stored_then_every_ten_minutes() {
        let now = Instant::now();
        assert!(own_key_due(None, now));
        assert!(!own_key_due(Some(now), now + Duration::from_secs(599)));
        assert!(own_key_due(Some(now), now + Duration::from_secs(600)));
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn requests_crossing_on_the_wire_make_both_friends() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::FriendRequest { peer: pb, note: String::new() }).unwrap();
        p.b.cmd_tx.send(Command::FriendRequest { peer: pa, note: String::new() }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::FriendAccepted { peer, .. } if *peer == pb)).await;
        wait_event(&mut p.b, |e| matches!(e, Event::FriendAccepted { peer, .. } if *peer == pa)).await;
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn presence_from_a_peer_we_asked_is_not_answered_with_a_remove() {
        // b accepted a's request while a was away; a still holds it as `out`.
        let mut p = pair(["{peer} out b\t\n", "{peer} friend a\t\n"]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        wait_event(&mut p.a, |e| matches!(e, Event::FriendAccepted { peer, .. } if *peer == pb)).await;
        assert!(no_event(&mut p.b, Duration::from_secs(1), |e| matches!(e, Event::FriendRemoved { peer } if *peer == pa)).await);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_plain_decline_removes_the_request_on_both_sides() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::FriendRequest { peer: pb, note: String::new() }).unwrap();
        wait_event(&mut p.b, |e| matches!(e, Event::FriendRequest { .. })).await;
        p.b.cmd_tx.send(Command::FriendDecline { peer: pa, block: false }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::FriendRemoved { peer } if *peer == pb)).await;
        let b_file = std::fs::read_to_string(p.root.join("b/friends.txt")).unwrap_or_default();
        assert!(!b_file.contains(&pa.to_base58()), "{b_file}");
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_request_to_an_unreachable_peer_fails_and_is_dropped() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-unreach-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut x = spawn(Keypair::generate_ed25519(), config(&root, "x")).await.unwrap();
        let nobody = Keypair::generate_ed25519().public().to_peer_id();
        x.cmd_tx.send(Command::FriendRequest { peer: nobody, note: String::new() }).unwrap();
        wait_event(&mut x, |e| matches!(e, Event::FriendRequestFailed { peer } if *peer == nobody)).await;
        let file = std::fs::read_to_string(root.join("x/friends.txt")).unwrap_or_default();
        assert!(!file.contains(&nobody.to_base58()), "{file}");
        x.cmd_tx.send(Command::Shutdown).unwrap();
        drop(x);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[test]
    fn friends_and_requests_away_from_us_are_looked_up_but_blocked_peers_are_not() {
        let dir = std::env::temp_dir().join(format!("rexnet-lookup-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut store = FriendStore::load(&dir);
        let peers: Vec<PeerId> = (0..5).map(|_| Keypair::generate_ed25519().public().to_peer_id()).collect();
        store.set(peers[0], Kind::Friend, None, None);
        store.set(peers[1], Kind::Out, None, None);
        store.set(peers[2], Kind::In, None, None);
        store.set(peers[3], Kind::Blocked, None, None);
        store.set(peers[4], Kind::Friend, None, None);
        let connected = HashSet::from([peers[4]]);
        let targets: HashSet<PeerId> = lookup_targets(&store, &[], &connected).into_iter().collect();
        assert_eq!(targets, HashSet::from([peers[0], peers[1], peers[2]]));
        assert_eq!(crate::protocol::peer_key(&peers[0]), format!("rexnet/v1/peer/{}", peers[0]));
        let _ = std::fs::remove_dir_all(dir);
    }

    #[test]
    fn peers_with_messages_waiting_are_looked_up_unless_blocked() {
        let dir = std::env::temp_dir().join(format!("rexnet-outbox-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut store = FriendStore::load(&dir);
        let id = || Keypair::generate_ed25519().public().to_peer_id();
        let (stranger, blocked, connected_peer) = (id(), id(), id());
        store.set(blocked, Kind::Blocked, None, None);
        let connected = HashSet::from([connected_peer]);
        let targets: HashSet<PeerId> =
            lookup_targets(&store, &[stranger, blocked, connected_peer], &connected).into_iter().collect();
        assert_eq!(targets, HashSet::from([stranger]));
        let _ = std::fs::remove_dir_all(dir);
    }

    fn club_update(team: u64) -> impl Fn(&Event) -> bool {
        move |e| matches!(e, Event::ClubUpdated { team: t, .. } if *t == team)
    }

    fn club_gone(team: u64, deleted: bool) -> impl Fn(&Event) -> bool {
        move |e| matches!(e, Event::ClubGone { team: t, deleted: d } if *t == team && *d == deleted)
    }

    /// `a` dials `b` on loopback and waits until connected.
    async fn connect(a: &mut EngineHandles, b: &EngineHandles) {
        let (address, pb) = (loopback_address(b).await, b.peer_id);
        a.cmd_tx.send(Command::ConnectManual { multiaddr: format!("{address}/p2p/{pb}") }).unwrap();
        wait_event(a, |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == pb)).await;
    }

    fn publish(team: u64, version: u64, record: &[u8], members: Vec<PeerId>) -> Command {
        Command::ClubPublish { team, version, record: record.to_vec(), members, name: None }
    }

    #[test]
    fn a_club_query_gets_record_unchanged_not_member_deleted_moved_or_unknown() {
        let dir = std::env::temp_dir().join(format!("rexnet-club-reply-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut store = crate::clubs::ClubStore::load(&dir);
        let id = || Keypair::generate_ed25519().public().to_peer_id();
        let (me, member, stranger, owner) = (id(), id(), id(), id());
        store.set_owned(1, &me, 3, b"r3".to_vec(), &[member], None, 0);
        let record = ClubReply::Record { version: 3, record: b"r3".to_vec(), members: vec![member.to_bytes()] };
        assert_eq!(club_reply(&store, &member, 1, 2), record);
        assert_eq!(club_reply(&store, &member, 1, 3), ClubReply::Unchanged);
        assert_eq!(club_reply(&store, &stranger, 1, 0), ClubReply::NotMember);
        assert_eq!(club_reply(&store, &member, 9, 0), ClubReply::Unknown);
        store.set_member(2, &owner, 1, b"x".to_vec(), &[me], 0);
        assert_eq!(club_reply(&store, &member, 2, 0), ClubReply::Moved { owner: owner.to_bytes() });
        store.mark_deleted(1, 0);
        assert_eq!(club_reply(&store, &member, 1, 0), ClubReply::Deleted);
        let _ = std::fs::remove_dir_all(dir);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_member_gets_the_owners_record_by_query_then_by_push() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(publish(7, 1, b"v1", vec![pb])).unwrap();
        settle(&p.a, pb).await;
        // b joins: its provisional copy gives way to the owner's record.
        p.b.cmd_tx.send(Command::ClubTrack { team: 7, owner: pa, record: b"mine".to_vec() }).unwrap();
        let Event::ClubUpdated { version, owned, record, owner, members, .. } =
            wait_event(&mut p.b, club_update(7)).await
        else {
            unreachable!()
        };
        assert_eq!((version, owned, record.as_slice(), owner, members), (1, false, &b"v1"[..], pa, vec![pb]));
        p.a.cmd_tx.send(publish(7, 2, b"v2", vec![pb])).unwrap();
        let Event::ClubUpdated { version, .. } = wait_event(&mut p.b, club_update(7)).await else { unreachable!() };
        assert_eq!(version, 2);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn removal_and_deletion_reach_a_connected_member() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(publish(8, 1, b"v1", vec![pb])).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::ClubTrack { team: 8, owner: pa, record: b"p".to_vec() }).unwrap();
        wait_event(&mut p.b, club_update(8)).await;
        p.a.cmd_tx.send(publish(8, 2, b"v2", vec![])).unwrap();
        wait_event(&mut p.b, club_gone(8, false)).await;
        p.a.cmd_tx.send(publish(9, 1, b"v1", vec![pb])).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::ClubTrack { team: 9, owner: pa, record: b"p".to_vec() }).unwrap();
        wait_event(&mut p.b, club_update(9)).await;
        p.a.cmd_tx.send(Command::ClubDelete { team: 9 }).unwrap();
        wait_event(&mut p.b, club_gone(9, true)).await;
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_push_from_anyone_but_the_owner_is_ignored() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let elsewhere = Keypair::generate_ed25519().public().to_peer_id();
        p.b.cmd_tx.send(Command::ClubTrack { team: 10, owner: elsewhere, record: b"p".to_vec() }).unwrap();
        // b has handled ClubTrack once it answers a local command (b may not
        // have registered a's connection yet, so no echo to a here).
        loopback_address(&p.b).await;
        p.a.cmd_tx.send(publish(10, 5, b"forged", vec![pb])).unwrap();
        assert!(no_event(&mut p.b, Duration::from_millis(500), club_update(10)).await);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_query_to_a_former_owner_is_sent_on_to_the_owner_it_names() {
        let p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        let elsewhere = Keypair::generate_ed25519().public().to_peer_id();
        // a holds team 11 only as a member of `elsewhere`'s club.
        p.a.cmd_tx.send(Command::ClubTrack { team: 11, owner: elsewhere, record: b"p".to_vec() }).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::ClubTrack { team: 11, owner: pa, record: b"p".to_vec() }).unwrap();
        let b_dir = p.root.join("b");
        tokio::time::timeout(Duration::from_secs(5), async {
            while crate::clubs::ClubStore::load(&b_dir).get(11).and_then(|c| c.owner()) != Some(elsewhere) {
                tokio::time::sleep(Duration::from_millis(20)).await;
            }
        })
        .await
        .expect("b never followed the move");
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn club_members_get_each_others_presence() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(publish(12, 1, b"v1", vec![pb])).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::ClubTrack { team: 12, owner: pa, record: b"p".to_vec() }).unwrap();
        wait_event(&mut p.b, club_update(12)).await;
        p.b.cmd_tx.send(Command::SetPresence { title_id: 0x11223344, state: 1, rich: vec![] }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::PresenceUpdated { peer, .. } if *peer == pb)).await;
        p.a.cmd_tx.send(Command::SetPresence { title_id: 0x11223344, state: 1, rich: vec![] }).unwrap();
        wait_event(&mut p.b, |e| matches!(e, Event::PresenceUpdated { peer, .. } if *peer == pa)).await;
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn clubs_are_replayed_at_startup() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-club-replay-{}-{}", std::process::id(), rand::random::<u64>()));
        std::fs::create_dir_all(root.join("a")).unwrap();
        let ka = Keypair::generate_ed25519();
        {
            let a = spawn(ka.clone(), config(&root, "a")).await.unwrap();
            a.cmd_tx.send(publish(13, 4, b"mine", vec![])).unwrap();
            tokio::time::timeout(Duration::from_secs(3), async {
                while crate::clubs::ClubStore::load(&root.join("a")).get(13).is_none() {
                    tokio::time::sleep(Duration::from_millis(20)).await;
                }
            })
            .await
            .unwrap();
            a.cmd_tx.send(Command::Shutdown).unwrap();
            tokio::time::sleep(Duration::from_millis(100)).await;
        }
        let mut a = spawn(ka, config(&root, "a")).await.unwrap();
        let Event::ClubUpdated { version, owned, record, .. } = wait_event(&mut a, club_update(13)).await else {
            unreachable!()
        };
        assert_eq!((version, owned, record.as_slice()), (4, true, &b"mine"[..]));
        a.cmd_tx.send(Command::Shutdown).unwrap();
        drop(a);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_member_removed_while_away_learns_it_on_reconnect() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-club-away-{}-{}", std::process::id(), rand::random::<u64>()));
        for name in ["a", "b"] {
            std::fs::create_dir_all(root.join(name)).unwrap();
        }
        let (ka, kb) = (Keypair::generate_ed25519(), Keypair::generate_ed25519());
        let (pa, pb) = (ka.public().to_peer_id(), kb.public().to_peer_id());
        let mut a = spawn(ka, config(&root, "a")).await.unwrap();
        let mut b = spawn(kb.clone(), config(&root, "b")).await.unwrap();
        connect(&mut a, &b).await;
        a.cmd_tx.send(publish(14, 1, b"v1", vec![pb])).unwrap();
        settle(&a, pb).await;
        b.cmd_tx.send(Command::ClubTrack { team: 14, owner: pa, record: b"p".to_vec() }).unwrap();
        wait_event(&mut b, club_update(14)).await;
        // b goes away; a removes b meanwhile.
        b.cmd_tx.send(Command::Shutdown).unwrap();
        drop(b);
        tokio::time::sleep(Duration::from_millis(100)).await;
        a.cmd_tx.send(publish(14, 2, b"v2", vec![])).unwrap();
        let mut b = spawn(kb, config(&root, "b")).await.unwrap();
        connect(&mut a, &b).await;
        wait_event(&mut b, club_gone(14, false)).await;
        a.cmd_tx.send(Command::Shutdown).unwrap();
        b.cmd_tx.send(Command::Shutdown).unwrap();
        drop((a, b));
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_club_request_reaches_an_owner_who_was_offline() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-club-request-{}-{}", std::process::id(), rand::random::<u64>()));
        for name in ["a", "b"] {
            std::fs::create_dir_all(root.join(name)).unwrap();
        }
        let (ka, kb) = (Keypair::generate_ed25519(), Keypair::generate_ed25519());
        let (pa, pb) = (ka.public().to_peer_id(), kb.public().to_peer_id());
        // b asks while a is not running: the request waits in b's outbox.
        let mut b = spawn(kb, config(&root, "b")).await.unwrap();
        b.cmd_tx.send(Command::ClubRequest { owner: pa, id: [8; 16], blob: b"promote".to_vec() }).unwrap();
        tokio::time::timeout(Duration::from_secs(3), async {
            while crate::mailbox::Mailbox::load(&root.join("b")).pending_for(&pa).is_empty() {
                tokio::time::sleep(Duration::from_millis(20)).await;
            }
        })
        .await
        .unwrap();
        let mut a = spawn(ka, config(&root, "a")).await.unwrap();
        connect(&mut b, &a).await;
        let Event::ClubRequest { peer, blob } = wait_event(&mut a, |e| matches!(e, Event::ClubRequest { .. })).await
        else {
            unreachable!()
        };
        assert_eq!((peer, blob.as_slice()), (pb, &b"promote"[..]));
        assert!(no_event(&mut a, Duration::from_millis(300), |e| matches!(e, Event::MessageReceived { .. })).await);
        a.cmd_tx.send(Command::Shutdown).unwrap();
        b.cmd_tx.send(Command::Shutdown).unwrap();
        drop((a, b));
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_handover_makes_the_new_owner_hold_the_club() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(publish(20, 1, b"v1", vec![pb])).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::ClubTrack { team: 20, owner: pa, record: b"p".to_vec() }).unwrap();
        wait_event(&mut p.b, club_update(20)).await;
        let handover =
            Command::ClubHandover { team: 20, new_owner: pb, id: [9; 16], version: 2, record: b"v2".to_vec(), members: vec![pa] };
        p.a.cmd_tx.send(handover).unwrap();
        let Event::ClubUpdated { owned, version, record, owner, members, .. } =
            wait_event(&mut p.b, club_update(20)).await
        else {
            unreachable!()
        };
        assert_eq!((owned, version, record.as_slice(), owner, members), (true, 2, &b"v2"[..], pb, vec![pa]));
        // a is now a member of b's club, and sends late askers on to b.
        let club = crate::clubs::ClubStore::load(&p.root.join("a")).get(20).cloned().unwrap();
        assert_eq!((club.role, club.owner(), club.version), (crate::clubs::Role::Member, Some(pb), 2));
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_handover_from_anyone_but_the_owner_is_dropped() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let handover =
            Command::ClubHandover { team: 21, new_owner: pb, id: [10; 16], version: 1, record: b"x".to_vec(), members: vec![] };
        p.a.cmd_tx.send(handover).unwrap();
        outbox_empties(p.root.join("a"), pb).await;
        assert!(no_event(&mut p.b, Duration::from_millis(300), club_update(21)).await);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_name_check_finds_another_owners_club_name() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let named = Command::ClubPublish { team: 22, version: 1, record: b"r".to_vec(), members: vec![], name: Some("Speed Kings".into()) };
        p.a.cmd_tx.send(named).unwrap();
        settle(&p.a, pb).await;
        p.b.cmd_tx.send(Command::ClubNameCheck { id: 1, name: "  speed KINGS ".into() }).unwrap();
        p.b.cmd_tx.send(Command::ClubNameCheck { id: 2, name: "Nobody".into() }).unwrap();
        let mut results = HashMap::new();
        tokio::time::timeout(Duration::from_secs(20), async {
            while results.len() < 2 {
                if let Event::ClubNameResult { id, taken } = p.b.evt_rx.recv().await.unwrap() {
                    results.insert(id, taken);
                }
            }
        })
        .await
        .expect("name checks never finished");
        assert_eq!(results, HashMap::from([(1, true), (2, false)]));
        finish(p).await;
    }

    #[test]
    fn a_pending_handover_goes_out_alone_before_anything_else_to_that_peer() {
        let pending = vec![([1; 16], 1, b"demote".to_vec()), ([2; 16], 2, b"handover".to_vec()), ([3; 16], 0, b"hi".to_vec())];
        assert_eq!(deliverable(pending.clone()), vec![([2; 16], 2, b"handover".to_vec())]);
        let rest: Vec<([u8; 16], u8, Vec<u8>)> = pending.into_iter().filter(|(_, kind, _)| *kind != 2).collect();
        assert_eq!(deliverable(rest.clone()), rest);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_connected_member_follows_a_transfer_when_the_new_owner_pushes() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        std::fs::create_dir_all(p.root.join("c")).unwrap();
        let mut c = spawn(Keypair::generate_ed25519(), config(&p.root, "c")).await.unwrap();
        let pc = c.peer_id;
        connect(&mut c, &p.a).await;
        connect(&mut c, &p.b).await;
        p.a.cmd_tx.send(publish(40, 1, b"v1", vec![pb, pc])).unwrap();
        settle(&p.a, pb).await;
        for member in [&p.b, &c] {
            member.cmd_tx.send(Command::ClubTrack { team: 40, owner: pa, record: b"p".to_vec() }).unwrap();
        }
        wait_event(&mut p.b, club_update(40)).await;
        wait_event(&mut c, club_update(40)).await;
        // a hands the club to b; b, now its owner, publishes the next version.
        let handover =
            Command::ClubHandover { team: 40, new_owner: pb, id: [11; 16], version: 2, record: b"v2".to_vec(), members: vec![pa, pc] };
        p.a.cmd_tx.send(handover).unwrap();
        wait_event(&mut p.b, |e| matches!(e, Event::ClubUpdated { team: 40, owned: true, .. })).await;
        p.b.cmd_tx.send(publish(40, 3, b"v3", vec![pa, pc])).unwrap();
        // c still thinks a owns it: b's push makes c ask a, who sends it on to b.
        let Event::ClubUpdated { owner, version, .. } = wait_event(&mut c, club_update(40)).await else { unreachable!() };
        assert_eq!((owner, version), (pb, 3));
        c.cmd_tx.send(Command::Shutdown).unwrap();
        drop(c);
        finish(p).await;
    }

    fn big_blob() -> Vec<u8> {
        (0..10_240u32).map(|i| (i % 251) as u8).collect()
    }

    /// Wait until `dir`'s saved outbox holds nothing for `peer`.
    async fn outbox_empties(dir: PathBuf, peer: PeerId) {
        tokio::time::timeout(Duration::from_secs(5), async {
            while !crate::mailbox::Mailbox::load(&dir).pending_for(&peer).is_empty() {
                tokio::time::sleep(Duration::from_millis(20)).await;
            }
        })
        .await
        .expect("outbox never emptied");
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_message_reaches_a_connected_peer_and_leaves_the_outbox() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        p.a.cmd_tx.send(Command::MessageSend { peer: pb, id: [1; 16], blob: big_blob() }).unwrap();
        let Event::MessageReceived { peer, blob } =
            wait_event(&mut p.b, |e| matches!(e, Event::MessageReceived { .. })).await
        else {
            unreachable!()
        };
        assert_eq!((peer, blob), (pa, big_blob()));
        outbox_empties(p.root.join("a"), pb).await;
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_message_saved_before_a_restart_is_delivered_once_its_peer_connects() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-msg-restart-{}-{}", std::process::id(), rand::random::<u64>()));
        let (ka, kb) = (Keypair::generate_ed25519(), Keypair::generate_ed25519());
        let (pa, pb) = (ka.public().to_peer_id(), kb.public().to_peer_id());
        for name in ["a", "b"] {
            std::fs::create_dir_all(root.join(name)).unwrap();
        }
        // a queues for b while b does not exist yet, then shuts down: the
        // message survives only in a's mailbox.bin.
        {
            let a = spawn(ka.clone(), config(&root, "a")).await.unwrap();
            a.cmd_tx.send(Command::MessageSend { peer: pb, id: [7; 16], blob: b"later".to_vec() }).unwrap();
            tokio::time::timeout(Duration::from_secs(3), async {
                while crate::mailbox::Mailbox::load(&root.join("a")).pending_for(&pb).is_empty() {
                    tokio::time::sleep(Duration::from_millis(20)).await;
                }
            })
            .await
            .unwrap();
            a.cmd_tx.send(Command::Shutdown).unwrap();
            tokio::time::sleep(Duration::from_millis(100)).await;
        }
        let mut b = spawn(kb, config(&root, "b")).await.unwrap();
        let a = spawn(ka, config(&root, "a")).await.unwrap();
        let address = loopback_address(&b).await;
        a.cmd_tx.send(Command::ConnectManual { multiaddr: format!("{address}/p2p/{pb}") }).unwrap();
        let Event::MessageReceived { peer, blob } =
            wait_event(&mut b, |e| matches!(e, Event::MessageReceived { .. })).await
        else {
            unreachable!()
        };
        assert_eq!((peer, blob.as_slice()), (pa, &b"later"[..]));
        outbox_empties(root.join("a"), pb).await;
        a.cmd_tx.send(Command::Shutdown).unwrap();
        b.cmd_tx.send(Command::Shutdown).unwrap();
        drop((a, b));
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_resent_message_is_received_once_but_a_new_one_arrives() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let received = |e: &Event| matches!(e, Event::MessageReceived { .. });
        p.a.cmd_tx.send(Command::MessageSend { peer: pb, id: [3; 16], blob: b"once".to_vec() }).unwrap();
        let Event::MessageReceived { blob, .. } = wait_event(&mut p.b, received).await else { unreachable!() };
        assert_eq!(blob, b"once");
        outbox_empties(p.root.join("a"), pb).await;
        // As if the acknowledgement had been lost: the same id again, then a new message.
        p.a.cmd_tx.send(Command::MessageSend { peer: pb, id: [3; 16], blob: b"once".to_vec() }).unwrap();
        p.a.cmd_tx.send(Command::MessageSend { peer: pb, id: [4; 16], blob: b"twice".to_vec() }).unwrap();
        let Event::MessageReceived { blob, .. } = wait_event(&mut p.b, received).await else { unreachable!() };
        assert_eq!(blob, b"twice", "the resend must not surface");
        // b acknowledges before a forgets, so an empty outbox means b handled both.
        outbox_empties(p.root.join("a"), pb).await;
        assert!(no_event(&mut p.b, Duration::from_millis(300), received).await);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn messages_from_a_blocked_peer_are_acknowledged_and_dropped() {
        let mut p = pair(["", "{peer} blocked spammer\t\n"]).await;
        let pb = p.b.peer_id;
        p.a.cmd_tx.send(Command::MessageSend { peer: pb, id: [5; 16], blob: b"buy".to_vec() }).unwrap();
        outbox_empties(p.root.join("a"), pb).await;
        assert!(no_event(&mut p.b, Duration::from_millis(500), |e| matches!(e, Event::MessageReceived { .. })).await);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn startup_replays_the_saved_list_with_names() {
        let _network = network_lock();
        let root =std::env::temp_dir().join(format!("rexnet-replay-{}-{}", std::process::id(), rand::random::<u64>()));
        let id = || Keypair::generate_ed25519().public().to_peer_id();
        let (f, i, o) = (id(), id(), id());
        std::fs::create_dir_all(root.join("x")).unwrap();
        std::fs::write(
            root.join("x/friends.txt"),
            format!("{f} friend Fred\t\n{i} in Ian\thello\n{o} out Olga\tbye\n"),
        )
        .unwrap();
        let mut x = spawn(Keypair::generate_ed25519(), config(&root, "x")).await.unwrap();
        let mut seen = (false, false, false);
        tokio::time::timeout(Duration::from_secs(5), async {
            while seen != (true, true, true) {
                match x.evt_rx.recv().await.unwrap() {
                    Event::FriendAccepted { peer, display_name } if peer == f => {
                        assert_eq!(display_name, "Fred");
                        seen.0 = true;
                    }
                    Event::FriendRequest { peer, display_name, note } if peer == i => {
                        assert_eq!((display_name.as_str(), note.as_str()), ("Ian", "hello"));
                        seen.1 = true;
                    }
                    Event::FriendRequestSent { peer, display_name } if peer == o => {
                        assert_eq!(display_name, "Olga");
                        seen.2 = true;
                    }
                    _ => {}
                }
            }
        })
        .await
        .unwrap();
        x.cmd_tx.send(Command::Shutdown).unwrap();
        drop(x);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_friend_lookup_stops_after_finding_its_target_provider() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-friend-provider-{}-{}",std::process::id(),rand::random::<u64>()));
        let engine = spawn(Keypair::generate_ed25519(),config(&root,"a")).await.unwrap();
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        let (reply, receive) = oneshot::channel();
        engine.cmd_tx.send(Command::FriendProviderForTest { peer, reply }).unwrap();
        let (unrelated_retained, still_searching) = receive.await.unwrap();
        engine.cmd_tx.send(Command::Shutdown).unwrap();
        drop(engine);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
        assert!(unrelated_retained,"another provider must not terminate a lookup for this peer");
        assert!(!still_searching,"the matching provider must stop the crawl and duplicate dials");
    }

    /// Lists the shard protocol, as every RexNet node does, and offers no relay hop.
    #[derive(NetworkBehaviour)]
    struct ShardSpeaker {
        identify: identify::Behaviour,
        shard: request_response::cbor::Behaviour<(), ()>,
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn an_inbound_connection_is_closed_after_identify_only_when_the_peer_is_not_rexnet() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        let b_address = loopback_address(&p.b).await.with(Protocol::P2p(pb));
        // a and b may hold relay reservations on each other, which keeps them
        // whatever they speak; this node is RexNet and nothing else.
        let mut speaker = bare_swarm(|key| ShardSpeaker {
            identify: identify::Behaviour::new(identify::Config::new("rexnet/0.1.0".into(), key.public())),
            shard: request_response::cbor::Behaviour::new(
                [(StreamProtocol::new(crate::protocol::SHARD), ProtocolSupport::Full)],
                request_response::Config::default(),
            ),
        });
        speaker.dial(b_address.clone()).unwrap();
        let speaker_closed = tokio::spawn(async move {
            let identified =
                |e: &SwarmEvent<ShardSpeakerEvent>| matches!(e, SwarmEvent::Behaviour(ShardSpeakerEvent::Identify(identify::Event::Received { .. })));
            assert!(swarm_event(&mut speaker, Duration::from_secs(5), identified).await, "never identified");
            swarm_event(&mut speaker, Duration::from_secs(4), |e| matches!(e, SwarmEvent::ConnectionClosed { .. })).await
        });
        let mut public = public_peer();
        public.dial(b_address).unwrap();
        assert!(swarm_event(&mut public, Duration::from_secs(5), |e| matches!(e, SwarmEvent::ConnectionEstablished { .. })).await);
        assert!(
            swarm_event(&mut public, Duration::from_secs(3), |e| matches!(e, SwarmEvent::ConnectionClosed { .. })).await,
            "b kept a public peer's inbound connection"
        );
        // a dialled b, so b holds a's connection as inbound: closing it would disconnect a.
        assert!(no_event(&mut p.a, Duration::from_secs(3), |e| matches!(e, Event::PeerDisconnected { peer } if *peer == pb)).await);
        settle(&p.b, pa).await;
        assert!(!speaker_closed.await.unwrap(), "b closed a RexNet node's inbound connection");
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn an_outbound_connection_to_a_public_peer_is_kept_after_identify() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-outbound-{}-{}", std::process::id(), rand::random::<u64>()));
        let engine = spawn(Keypair::generate_ed25519(), config(&root, "a")).await.unwrap();
        let mut public = public_peer();
        let address = bare_listen(&mut public).await;
        engine.cmd_tx.send(Command::ConnectManual { multiaddr: address.to_string() }).unwrap();
        let identified = |e: &SwarmEvent<identify::Event>| matches!(e, SwarmEvent::Behaviour(identify::Event::Received { .. }));
        assert!(swarm_event(&mut public, Duration::from_secs(5), identified).await, "never identified");
        assert!(
            !swarm_event(&mut public, Duration::from_secs(3), |e| matches!(e, SwarmEvent::ConnectionClosed { .. })).await,
            "the engine closed its own dial"
        );
        engine.cmd_tx.send(Command::Shutdown).unwrap();
        drop(engine);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[test]
    fn an_auto_relay_is_never_a_bootstrap_peer_or_one_backing_off_and_rexnet_relays_come_first() {
        let start = Instant::now();
        let waited = start + AUTO_RELAY_REXNET_WAIT;
        let eligible = |bootstrap, backoff, rexnet, holding_rexnet, now| {
            auto_relay_eligible(bootstrap, backoff, rexnet, holding_rexnet, start, now)
        };
        assert!(eligible(false, None, true, false, start), "a RexNet relay at once");
        assert!(eligible(false, None, true, true, start), "a second RexNet relay too");
        assert!(!eligible(true, None, true, false, waited), "never a bootstrap peer");
        assert!(!eligible(false, Some(waited), true, false, start), "not while it backs off");
        assert!(eligible(false, Some(waited), true, false, waited), "once its backoff is over");
        assert!(!eligible(false, None, false, false, start), "another relay waits for a RexNet one");
        assert!(eligible(false, None, false, false, waited), "and is taken when none came");
        assert!(!eligible(false, None, false, true, waited), "but not while a RexNet relay is held");
    }

    #[test]
    fn an_auto_relay_is_reserved_through_its_own_address_not_a_circuit_it_holds() {
        let [relay, other] = [(); 2].map(|_| Keypair::generate_ed25519().public().to_peer_id());
        let addr = |text: String| text.parse::<Multiaddr>().unwrap();
        let lan = addr("/ip4/192.168.0.2/tcp/4001".into());
        let held = addr(format!("/ip4/5.6.7.8/udp/4001/quic-v1/p2p/{other}/p2p-circuit/p2p/{relay}"));
        let direct = addr("/ip4/1.2.3.4/tcp/4001".into());
        assert_eq!(reservable_addr(&[lan.clone(), held.clone(), direct.clone()]), Some(&direct));
        assert_eq!(reservable_addr(&[held, lan]), None);
    }

    /// A RexNet node (it lists the shard protocol) whose relay server takes or
    /// refuses every reservation, advertising a public address.
    #[derive(NetworkBehaviour)]
    struct TestRelay {
        identify: identify::Behaviour,
        relay: relay::Behaviour,
        shard: request_response::cbor::Behaviour<(), ()>,
    }

    fn test_relay(accept: bool) -> Swarm<TestRelay> {
        let mut swarm = bare_swarm(|key| {
            let mut config = relay::Config::default();
            if !accept {
                config.max_reservations = 0;
            }
            TestRelay {
                identify: identify::Behaviour::new(identify::Config::new("rexnet/0.1.0".into(), key.public())),
                relay: relay::Behaviour::new(key.public().to_peer_id(), config),
                shard: request_response::cbor::Behaviour::new(
                    [(StreamProtocol::new(crate::protocol::SHARD), ProtocolSupport::Full)],
                    request_response::Config::default(),
                ),
            }
        });
        // Auto relays are reserved only through a global address. The
        // reservation itself goes over the loopback connection already open.
        swarm.add_external_address("/ip4/1.2.3.4/tcp/4001".parse().unwrap());
        swarm
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_relay_that_refuses_our_reservation_frees_its_slot_at_once() {
        let _network = network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-relay-{}-{}", std::process::id(), rand::random::<u64>()));
        let engine = spawn(Keypair::generate_ed25519(), config(&root, "a")).await.unwrap();
        // The refusing relays stay connected, so without the fix they hold
        // every slot and the last relay is never asked.
        for (i, accept) in std::iter::repeat_n(false, MAX_AUTO_RELAYS).chain([true]).enumerate() {
            let mut relay = test_relay(accept);
            let address = bare_listen(&mut relay).await;
            engine.cmd_tx.send(Command::Dial { multiaddr: address.to_string() }).unwrap();
            let answer = tokio::time::timeout(Duration::from_secs(5), async {
                loop {
                    match relay.select_next_some().await {
                        SwarmEvent::Behaviour(TestRelayEvent::Relay(relay::Event::ReservationReqAccepted { .. })) => break true,
                        SwarmEvent::Behaviour(TestRelayEvent::Relay(relay::Event::ReservationReqDenied { .. })) => break false,
                        _ => {}
                    }
                }
            })
            .await;
            assert_eq!(answer, Ok(accept), "relay {i}");
            tokio::spawn(async move {
                loop {
                    relay.select_next_some().await;
                }
            });
            // Time for the engine to see the refusal before the next relay identifies.
            tokio::time::sleep(Duration::from_millis(300)).await;
        }
        engine.cmd_tx.send(Command::Shutdown).unwrap();
        drop(engine);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }
}

#[cfg(test)]
mod notice_tests {
    use super::friend_tests::{bare_listen, bare_swarm, config, finish, loopback_address, no_event, pair, wait_event};
    use super::*;
    use crate::notice::{sign, test_keypair, Ask, Kind, Notice, Slot};

    fn notice(kind: Kind, to: Vec<Vec<u8>>) -> Notice {
        Notice {
            schema: 1,
            seq: crate::notice::unix_ms(),
            kind,
            not_after: crate::mailbox::unix_now() + 600,
            built_before: None,
            to,
            titles: Vec::new(),
            style: None,
        }
    }

    /// Wait until `handles` shows something in `slot`.
    async fn shows(handles: &EngineHandles, slot: Slot) -> Notice {
        tokio::time::timeout(Duration::from_secs(8), async {
            loop {
                let now = crate::mailbox::unix_now();
                if let Some(shown) = handles.notices.lock().unwrap().visible(slot, now, "0.0.1") {
                    return shown;
                }
                tokio::time::sleep(Duration::from_millis(50)).await;
            }
        })
        .await
        .expect("notice never arrived")
    }

    fn shows_nothing(handles: &EngineHandles) -> bool {
        let now = crate::mailbox::unix_now();
        let slots = handles.notices.lock().unwrap();
        slots.visible(Slot::Broadcast, now, "0.0.1").is_none() && slots.visible(Slot::Personal, now, "0.0.1").is_none()
    }

    /// A third engine that dials `to` after the fact.
    async fn joiner(root: &std::path::Path, to: &EngineHandles, key: Keypair) -> EngineHandles {
        let mut c = spawn(key, config(root, "c")).await.unwrap();
        let address = loopback_address(to).await;
        c.cmd_tx.send(Command::ConnectManual { multiaddr: format!("{address}/p2p/{}", to.peer_id) }).unwrap();
        wait_event(&mut c, |e| matches!(e, Event::PeerConnected { peer, .. } if *peer == to.peer_id)).await;
        c
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_broadcast_reaches_a_connected_peer_and_a_later_joiner() {
        let mut p = pair(["", ""]).await;
        let dvd = notice(Kind::Dvd, vec![]);
        p.a.cmd_tx.send(Command::PublishNotice { signed: sign(&test_keypair(), &dvd) }).unwrap();
        assert_eq!(shows(&p.b, Slot::Broadcast).await, dvd);
        assert!(no_event(&mut p.a, Duration::from_secs(1), |e| matches!(e, Event::NoticeDelivered { .. })).await, "a forward or hand-off is not a delivery");
        let c = joiner(&p.root, &p.b, Keypair::generate_ed25519()).await;
        assert_eq!(shows(&c, Slot::Broadcast).await, dvd, "hand-off on identify");
        c.cmd_tx.send(Command::Shutdown).unwrap();
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_broadcast_for_another_title_is_held_by_its_publisher_but_not_adopted() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let mut scoped = notice(Kind::Dvd, vec![]);
        scoped.titles = vec![0xDEAD_BEEF];
        let signed = sign(&test_keypair(), &scoped);
        p.a.cmd_tx.send(Command::PublishNotice { signed: signed.clone() }).unwrap();
        // Sent to b directly as well: its Ack comes after b has decided, so b
        // has certainly seen the notice.
        p.a.cmd_tx.send(Command::SendNoticeTo { peer: pb, signed: signed.clone() }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::NoticeDelivered { peer } if *peer == pb)).await;
        let now = crate::mailbox::unix_now();
        assert_eq!(p.a.notices.lock().unwrap().handoff(now), Some(signed), "the publisher is exempt");
        assert_eq!(p.b.notices.lock().unwrap().handoff(now), None, "b runs 11223344, not DEADBEEF");
        assert!(shows_nothing(&p.b));
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_targeted_notice_is_acknowledged_and_never_passed_on() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let kc = Keypair::generate_ed25519();
        let pc = kc.public().to_peer_id();
        let text = notice(Kind::Text("hi b".into()), vec![pb.to_bytes(), pc.to_bytes()]);
        p.a.cmd_tx.send(Command::SendNoticeTo { peer: pb, signed: sign(&test_keypair(), &text) }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::NoticeDelivered { peer } if *peer == pb)).await;
        assert_eq!(shows(&p.b, Slot::Personal).await, text);
        assert!(shows_nothing(&p.a), "the sender is not a target");
        let c = joiner(&p.root, &p.b, kc).await;
        tokio::time::sleep(Duration::from_secs(2)).await;
        assert!(shows_nothing(&c), "a personal notice is never passed on");
        c.cmd_tx.send(Command::Shutdown).unwrap();
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_receiver_refuses_a_notice_signed_by_another_key() {
        let mut p = pair(["", ""]).await;
        let pb = p.b.peer_id;
        let forged = sign(&Keypair::generate_ed25519(), &notice(Kind::Dvd, vec![]));
        p.a.cmd_tx.send(Command::SendNoticeTo { peer: pb, signed: forged }).unwrap();
        wait_event(&mut p.a, |e| matches!(e, Event::NoticeDelivered { peer } if *peer == pb)).await;
        assert!(shows_nothing(&p.b));
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_roster_ask_is_answered_only_when_signed_by_the_key_and_sent_by_its_panel() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        let ask = |key: &Keypair, panel: PeerId| {
            sign(key, &Ask { schema: 1, panel_peer: panel.to_bytes(), issued_ms: crate::notice::unix_ms() })
        };
        // The good ask first: its answer proves b is a notice peer of a, so
        // the refusals below are real refusals, not unsent asks.
        let deadline = tokio::time::Instant::now() + Duration::from_secs(8);
        let (display_name, title_id) = loop {
            p.a.cmd_tx.send(Command::Roster { title_id: 0x11223344, ask: ask(&test_keypair(), pa), refresh: true }).unwrap();
            let answer = tokio::time::timeout(Duration::from_millis(500), async {
                loop {
                    if let Event::RosterEntry { peer, display_name, title_id, .. } = p.a.evt_rx.recv().await.expect("engine stopped") {
                        if peer == pb {
                            return (display_name, title_id);
                        }
                    }
                }
            })
            .await;
            match answer {
                Ok(found) => break found,
                Err(_) => assert!(tokio::time::Instant::now() < deadline, "roster never answered"),
            }
        };
        assert_eq!((display_name.as_str(), title_id), ("b", 0x11223344));
        // A resent good ask may still be answered; drain it so the refusals below are clean.
        tokio::time::sleep(Duration::from_secs(1)).await;
        while p.a.evt_rx.try_recv().is_ok() {}
        p.a.cmd_tx.send(Command::Roster { title_id: 0x11223344, ask: ask(&Keypair::generate_ed25519(), pa), refresh: true }).unwrap();
        assert!(no_event(&mut p.a, Duration::from_secs(2), |e| matches!(e, Event::RosterEntry { .. })).await, "wrong key");
        p.a.cmd_tx.send(Command::Roster { title_id: 0x11223344, ask: ask(&test_keypair(), pb), refresh: true }).unwrap();
        assert!(no_event(&mut p.a, Duration::from_secs(2), |e| matches!(e, Event::RosterEntry { .. })).await, "another panel's ask");
        finish(p).await;
    }

    #[test]
    fn only_a_peer_with_no_notice_protocol_is_reported_as_an_old_build() {
        use request_response::OutboundFailure as Failure;
        let peer = Keypair::generate_ed25519().public().to_peer_id();
        assert!(matches!(
            old_build_event(&Failure::UnsupportedProtocols, peer, 0x5454_0079),
            Some(Event::RosterOldBuild { peer: p, title_id: 0x5454_0079 }) if p == peer
        ));
        // Unreachable is not old: only a connected peer can refuse the protocol.
        for other in [Failure::DialFailure, Failure::Timeout, Failure::ConnectionClosed, Failure::Io(std::io::Error::other("x"))] {
            assert!(old_build_event(&other, peer, 1).is_none(), "{other:?}");
        }
    }

    #[test]
    fn a_peer_is_asked_or_dialled_once_per_round() {
        let [a, b, c] = [(); 3].map(|_| Keypair::generate_ed25519().public().to_peer_id());
        let mut asked = HashSet::new();
        assert_eq!(unasked(&mut asked, [a, b, a]), vec![a, b], "repeats within one batch");
        assert_eq!(unasked(&mut asked, [b, c, a]), vec![c], "a later kad step re-lists the same providers");
        assert!(unasked(&mut asked, [a, b, c]).is_empty());
        asked.clear(); // the next Command::Roster
        assert_eq!(unasked(&mut asked, [a]), vec![a]);
    }

    #[test]
    fn a_roster_address_is_the_newest_dialled_link_ending_in_its_peer() {
        let [peer, relay, other] = [(); 3].map(|_| Keypair::generate_ed25519().public().to_peer_id());
        let link = |id, remote: Option<String>| Link {
            id: ConnectionId::new_unchecked(id),
            relayed: false,
            quic: false,
            renew_at: None,
            renew_bytes: None,
            inbound: remote.is_none(),
            remote: remote.map(|text| text.parse().unwrap()),
        };
        let circuit = format!("/ip4/217.216.53.41/tcp/4001/p2p/{relay}/p2p-circuit/p2p/{peer}");
        let links = [link(1, Some("/ip4/1.2.3.4/tcp/4001".into())), link(2, Some(circuit.clone())), link(3, None)];
        assert_eq!(roster_addr(&links, peer), Some(circuit), "the newest link we dialled, a circuit as it is");
        assert_eq!(roster_addr(&links[..1], peer), Some(format!("/ip4/1.2.3.4/tcp/4001/p2p/{peer}")), "/p2p/<peer> appended");
        assert_eq!(roster_addr(&[link(4, Some(format!("/ip4/1.2.3.4/tcp/1/p2p/{other}")))], peer), None, "another peer's");
        assert_eq!(roster_addr(&links[2..], peer), None, "only an accepted link");
        assert_eq!(roster_addr(&[], peer), None);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_stored_roster_ask_is_answered_when_a_runner_identifies_and_its_departure_is_reported() {
        let _network = super::friend_tests::network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-roster-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut a = spawn(Keypair::generate_ed25519(), config(&root, "a")).await.unwrap();
        let ask = sign(&test_keypair(), &Ask { schema: 1, panel_peer: a.peer_id.to_bytes(), issued_ms: crate::notice::unix_ms() });
        a.cmd_tx.send(Command::Roster { title_id: 0x11223344, ask, refresh: false }).unwrap();
        // b starts after the round began, so only identify can get it asked.
        let b = spawn(Keypair::generate_ed25519(), config(&root, "b")).await.unwrap();
        let pb = b.peer_id;
        let address = loopback_address(&b).await;
        a.cmd_tx.send(Command::ConnectManual { multiaddr: format!("{address}/p2p/{pb}") }).unwrap();
        let entry = wait_event(&mut a, |e| matches!(e, Event::RosterEntry { peer, .. } if *peer == pb)).await;
        let Event::RosterEntry { display_name, addr, .. } = entry else { unreachable!() };
        assert_eq!(display_name, "b");
        let addr = addr.expect("the address a dialled");
        assert!(addr.ends_with(&format!("/p2p/{pb}")), "{addr}");
        b.cmd_tx.send(Command::Shutdown).unwrap();
        wait_event(&mut a, |e| matches!(e, Event::RosterGone { peer } if *peer == pb)).await;
        a.cmd_tx.send(Command::Shutdown).unwrap();
        drop((a, b));
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_scan_skips_a_runner_already_reported_and_a_refresh_asks_it_again() {
        let mut p = pair(["", ""]).await;
        let (pa, pb) = (p.a.peer_id, p.b.peer_id);
        let roster = |refresh| Command::Roster {
            title_id: 0x11223344,
            ask: sign(&test_keypair(), &Ask { schema: 1, panel_peer: pa.to_bytes(), issued_ms: crate::notice::unix_ms() }),
            refresh,
        };
        let answered = |e: &Event| matches!(e, Event::RosterEntry { peer, .. } if *peer == pb);
        p.a.cmd_tx.send(roster(false)).unwrap();
        wait_event(&mut p.a, answered).await;
        p.a.cmd_tx.send(roster(false)).unwrap();
        assert!(no_event(&mut p.a, Duration::from_secs(2), answered).await, "a scan does not re-ask a reported runner");
        p.a.cmd_tx.send(roster(true)).unwrap();
        wait_event(&mut p.a, answered).await;
        finish(p).await;
    }

    /// Answers every notice request with an identity, asked for or not.
    #[derive(NetworkBehaviour)]
    struct Liar {
        identify: identify::Behaviour,
        notice: request_response::cbor::Behaviour<NoticeRequest, NoticeReply>,
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn an_identity_reply_to_a_notice_is_not_a_roster_entry() {
        let _network = super::friend_tests::network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-unasked-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut panel = spawn(Keypair::generate_ed25519(), config(&root, "a")).await.unwrap();
        let mut liar = bare_swarm(|key| Liar {
            identify: identify::Behaviour::new(identify::Config::new("rexnet/0.1.0".into(), key.public())),
            notice: request_response::cbor::Behaviour::new(
                [(StreamProtocol::new(crate::protocol::NOTICE), ProtocolSupport::Full)],
                request_response::Config::default(),
            ),
        });
        let address = bare_listen(&mut liar).await;
        let liar_peer = *liar.local_peer_id();
        let (replied, mut replies) = mpsc::unbounded_channel();
        tokio::spawn(async move {
            loop {
                match liar.select_next_some().await {
                    SwarmEvent::Behaviour(LiarEvent::Notice(request_response::Event::Message {
                        message: request_response::Message::Request { channel, .. },
                        ..
                    })) => {
                        let identity = RosterIdentity { display_name: "liar".into(), title_id: 0x11223344 };
                        let _ = liar.behaviour_mut().notice.send_response(channel, NoticeReply::Identity(Some(identity)));
                    }
                    SwarmEvent::Behaviour(LiarEvent::Notice(request_response::Event::ResponseSent { .. })) => {
                        let _ = replied.send(());
                    }
                    _ => {}
                }
            }
        });
        panel.cmd_tx.send(Command::ConnectManual { multiaddr: address.to_string() }).unwrap();
        let signed = sign(&test_keypair(), &notice(Kind::Dvd, vec![]));
        panel.cmd_tx.send(Command::SendNoticeTo { peer: liar_peer, signed }).unwrap();
        tokio::time::timeout(Duration::from_secs(8), replies.recv()).await.expect("the notice was never sent");
        assert!(no_event(&mut panel, Duration::from_secs(1), |e| matches!(e, Event::RosterEntry { .. })).await);
        panel.cmd_tx.send(Command::Shutdown).unwrap();
        drop(panel);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    /// A runner's shard discovery done by hand, since an engine reports
    /// neither the providers it finds nor a None answer: a RexNet node (its
    /// identify lists the shard protocol) with kad and shard queries.
    #[derive(NetworkBehaviour)]
    struct Prober {
        identify: identify::Behaviour,
        kad: kad::Behaviour<MemoryStore>,
        shard: request_response::cbor::Behaviour<ShardQuery, Option<ShardDesc>>,
    }

    /// Drive `probe` until `pick` takes an event.
    async fn probe_until<T>(probe: &mut Swarm<Prober>, mut pick: impl FnMut(SwarmEvent<ProberEvent>) -> Option<T>) -> T {
        tokio::time::timeout(Duration::from_secs(8), async {
            loop {
                if let Some(taken) = pick(probe.select_next_some().await) {
                    return taken;
                }
            }
        })
        .await
        .expect("expected probe event timed out")
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_beacon_is_a_shard_key_provider_that_answers_shard_queries_with_none() {
        let p = pair(["", ""]).await;
        let pa = p.a.peer_id;
        // a beacons the title b runs; b turns its shard on, as runners do.
        p.a.cmd_tx.send(Command::Beacon { title_id: 0x11223344 }).unwrap();
        p.a.cmd_tx.send(Command::Beacon { title_id: 0x11223344 }).unwrap();
        p.b.cmd_tx.send(Command::ShardEnable { cap: 0 }).unwrap();
        let mut probe = bare_swarm(|key| Prober {
            identify: identify::Behaviour::new(identify::Config::new("rexnet/0.1.0".into(), key.public())),
            kad: kad::Behaviour::new(key.public().to_peer_id(), MemoryStore::new(key.public().to_peer_id())),
            shard: request_response::cbor::Behaviour::new(
                [(StreamProtocol::new(crate::protocol::SHARD), ProtocolSupport::Full)],
                request_response::Config::default(),
            ),
        });
        probe.behaviour_mut().kad.add_address(&pa, loopback_address(&p.a).await);
        probe.behaviour_mut().kad.get_providers(kad::RecordKey::new(&crate::protocol::shard_key(0x11223344)));
        let found = probe_until(&mut probe, |event| match event {
            SwarmEvent::Behaviour(ProberEvent::Kad(kad::Event::OutboundQueryProgressed {
                result: kad::QueryResult::GetProviders(result),
                step,
                ..
            })) => {
                let found = matches!(&result, Ok(kad::GetProvidersOk::FoundProviders { providers, .. }) if providers.contains(&pa));
                (found || step.last).then_some(found)
            }
            _ => None,
        })
        .await;
        assert!(found, "a GetProviders for the title's shard key finds the beacon");
        probe.behaviour_mut().shard.send_request(&pa, ShardQuery { schema: 1, title_id: 0x11223344 });
        let answer = probe_until(&mut probe, |event| match event {
            SwarmEvent::Behaviour(ProberEvent::Shard(request_response::Event::Message {
                message: request_response::Message::Response { response, .. },
                ..
            })) => Some(Ok(response)),
            SwarmEvent::Behaviour(ProberEvent::Shard(request_response::Event::OutboundFailure { error, .. })) => {
                Some(Err(error.to_string()))
            }
            _ => None,
        })
        .await;
        assert_eq!(answer, Ok(None), "the beacon holds no shard to offer");
        let shard_of = |handles: &EngineHandles| {
            let (reply, receive) = oneshot::channel();
            handles.cmd_tx.send(Command::ShardStatus { reply }).unwrap();
            receive
        };
        assert_eq!(shard_of(&p.a).await.unwrap(), None, "a beacon never joins a shard");
        assert_eq!(shard_of(&p.b).await.unwrap(), None, "b's discovery found no shard at a to join");
        drop(probe);
        finish(p).await;
    }

    #[tokio::test(flavor = "multi_thread", worker_threads = 2)]
    async fn a_remembered_player_that_cannot_be_reached_is_not_reported() {
        let _network = super::friend_tests::network_lock();
        let root = std::env::temp_dir().join(format!("rexnet-dial-{}-{}", std::process::id(), rand::random::<u64>()));
        let mut panel = spawn(Keypair::generate_ed25519(), config(&root, "a")).await.unwrap();
        // No transport speaks bare UDP, so this dial fails at once.
        let stale = format!("/ip4/127.0.0.1/udp/9/p2p/{}", Keypair::generate_ed25519().public().to_peer_id());
        panel.cmd_tx.send(Command::Dial { multiaddr: stale.clone() }).unwrap();
        assert!(no_event(&mut panel, Duration::from_secs(2), |e| matches!(e, Event::Error { .. })).await, "a cache dial fails quietly");
        panel.cmd_tx.send(Command::ConnectManual { multiaddr: stale }).unwrap();
        wait_event(&mut panel, |e| matches!(e, Event::Error { .. })).await;
        panel.cmd_tx.send(Command::Shutdown).unwrap();
        drop(panel);
        tokio::time::sleep(Duration::from_millis(50)).await;
        let _ = std::fs::remove_dir_all(root);
    }

    #[test]
    fn a_title_has_one_seed_and_one_roster_lookup_running_at_a_time() {
        let local = Keypair::generate_ed25519().public().to_peer_id();
        let mut kad = kad::Behaviour::new(local, MemoryStore::new(local));
        let query = kad.get_providers(kad::RecordKey::new(&crate::protocol::shard_key(0x5454_0079)));
        let seeds = HashMap::from([(query, (0x5454_0079, HashSet::<PeerId>::new()))]);
        assert!(lookup_running(&seeds, 0x5454_0079));
        assert!(!lookup_running(&seeds, 0x4D53_0064), "another title's lookup");
        assert!(!lookup_running(&HashMap::<kad::QueryId, (u32, ())>::new(), 0x5454_0079));
    }
}
