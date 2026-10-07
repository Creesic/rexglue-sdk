# RexNet changes for original Xbox: review guide

This public review branch shares the RexNet work developed in the separate
`Creesic/xrecomp` project, without publishing a game
image or mixing in its renderer, generated game code, or guest XDK adapters.
It is based directly on Loreaxe's `netplay` branch at
[`3fcb0ddf4ff9263c991be0a8473d2bf10d456cd8`](https://github.com/Loreaxe/rexglue-sdk/commit/3fcb0ddf4ff9263c991be0a8473d2bf10d456cd8).
The SDK's upstream history and original RexNet source paths are retained.

[Review the complete diff against that base](https://github.com/Loreaxe/rexglue-sdk/compare/3fcb0ddf4ff9263c991be0a8473d2bf10d456cd8...Creesic:codex/rexnet-original-xbox).

## Provenance and scope

The exported runtime source is from xrecomp's `MC3` branch, commit
`4eecf52c9b32d98b8908925cf9b0e3b0c9d4f382`,
captured on 2026-10-06. Those source/dependency directories had no uncommitted
changes at export time. All 46 exported source and dependency files were
compared by SHA-256 against the local snapshot before building.

| xrecomp source | Location in this branch |
|---|---|
| `thirdparty/rexnet-core/src/` | `src/rexnet/core/src/` |
| `thirdparty/rexnet-core/examples/` | `src/rexnet/core/examples/` |
| `thirdparty/quinn-udp/` | `src/rexnet/thirdparty/quinn-udp/` |
| `thirdparty/quick-protobuf-codec/` | `src/rexnet/thirdparty/quick-protobuf-codec/` |

The manifest retains upstream's static library, CLI, optional `net-tests`, and
C-header generation. Its dependencies and path patches match the exported
implementation. The lockfile was seeded from xrecomp's lockfile, then resolved
for this standalone crate and upstream's retained header generator. Existing
integration tests receive the new required `EngineConfig.notice_key` field;
an all-zero key disables notices in those tests.

## Suggested review order

### Transport and recovery

Start with `core/src/engine.rs` and `core/src/game_plane.rs`:

- Relay discovery prefers RexNet peers, excludes bootstrap-only peers, frees
  refused reservation slots, and bounds automatic reservations.
- Inbound non-RexNet control connections are closed after protocol identify
  so public DHT traffic cannot occupy the entire inbound budget.
- Game sends queue while authentication/punching finishes; stale datagrams
  expire, and authenticated tunnels provide the fallback path.
- Last-control-connection loss starts bounded peer recovery: cached validated
  addresses, immediate dial, TCP/relay preference after QUIC failure, a single
  DHT lookup when needed, and a five-second game-peer grace period. Recovery
  requires a fresh authenticated game path, not just a control connection.
- Physical disconnect destroys old sockets, writers, keys, and replay state.
  Writer generations prevent a stale task from retiring its replacement.

The included `quinn-udp` patch retains version 0.5.15 and backports
[upstream's Windows ECN compatibility fix](https://github.com/quinn-rs/quinn/commit/e5b30f51341c323ca3fbaa4006912b105efc1018),
with follow-ups for Winsock ancillary pointers and IPv4 `IPV6_V6ONLY` queries.
Its `README.xrecomp.md` describes the patch and real-loopback regression test.

### Diagnostics

`core/src/diagnostics.rs` adds bounded network flight recording and health
records. Records distinguish submission, serialization, transport, receipt,
and queueing; sending or enqueueing is not proof of remote delivery. They
exclude packet payloads, secret keys, chat/voice contents, and save contents.

The `quick-protobuf-codec` patch measures the actual encoder/decoder without
changing its wire encoding or acceptance rules. The observer reports sizes,
completion states, message types, and DHT opcode metadata, not message bodies.
Its README records upstream provenance and the measurement limits.

### Social, notices, and population extensions

- `friends.rs` and `engine.rs`: named friends, request state, notes, decline,
  block, peer reachability discovery, presence, and revocable invitations.
- `mailbox.rs`: bounded store-and-forward title messages and attachments.
- `clubs.rs`: persisted club records, membership operations, and name checks.
- `notice.rs`, `engine.rs`, and `bin/rexnet-notice.rs`: signed developer notices,
  roster discovery, targeting, text styles/motion, and a persistent operator
  daemon. The embedded notice verification key is public; signing keys are
  external files and are not included in this branch.
- `population.rs` and `examples/population_probe.rs`: optional anonymous
  heartbeat reporting independent of the libp2p engine. Reporting failure does
  not block game networking.
- `ffi.rs`: C ABI entry points and events for these operations and diagnostics.

## Compatibility boundary

This is the original-Xbox fork's implementation for review, not a completed
port of those changes into the SDK's Xbox 360 C++ XAM shim. That shim is retained
from the base commit and needs adaptation to changed FFI signatures (for
example, `rexnet_send_invite` now takes a note argument). The workflow's SDK
job is skipped specifically on this review branch; core verification is kept.
The fork's friend store, social records, protocols, and extended FFI must be reviewed
before adopting them with existing upstream clients; shared protocol names
alone are not a compatibility guarantee.

The guest XNet/XOnline adapters, Plume overlays, Python notice GUI, heartbeat
collector, and title-specific consumers remain in xrecomp. That repository is
private; this branch contains the complete exported core and its dependency
patches so the implementation can be reviewed and tested independently. The
adapter supplies guest socket handles and ABI marshalling; games retain their
own lobby and simulation protocols. RexNet transports those games' datagrams,
and does not itself synchronize the simulation.

## Validation

Run from the repository root, with a Rust toolchain and seven or fewer build
jobs:

```sh
cargo test --locked --manifest-path src/rexnet/core/Cargo.toml -j 7
cargo test --locked --manifest-path src/rexnet/core/Cargo.toml --features net-tests -j 7 -- --test-threads=1
cargo test --locked --manifest-path src/rexnet/core/Cargo.toml -p quick-protobuf-codec -j 7
cargo test --locked --manifest-path src/rexnet/thirdparty/quinn-udp/Cargo.toml -j 7 -- --test-threads=1
```

The real-engine tests bind local sockets, including the controlled QUIC
blackhole/TCP-recovery case. They do not need public DHT access or a game image.
They establish local transport behavior, not internet/NAT reliability or
successful gameplay. The full SDK C++ build and live games are outside this
publication's validation scope.

Fresh Windows publication checks on 2026-10-06 passed:

| Check | Result |
|---|---|
| Compile every core, binary, example, and `net-tests` target | Passed |
| Core unit and real-engine checks, including QUIC blackhole/TCP recovery | 197 passed |
| Notice operator tool checks | 27 passed |
| Retained upstream loopback integration checks | 11 passed |
| Protobuf codec regression check | 1 passed |
| Quinn compatibility and UDP checks | 9 passed |
| C ABI header regenerated a second time | Byte-identical |
| Exported source/dependencies compared with the source snapshot | 46 files matched |
| Staged whitespace and publication-scope checks | Passed |

There were **245 passing tests**, with no failures or ignored tests. Test logs
remain local under the ignored `build/rexnet-publication/` directory; captured
player logs and signing keys are not part of the publication. These are local
Windows results. Linux CI, the SDK C++ adapter, and live gameplay are not claimed
as validated by this export.
