# quick-protobuf-codec 0.3.1

Runtime source from the published crate, bound to rust-libp2p commit
[`ce24938048cdb9c29f0b8218364557783cecd44a`](https://github.com/libp2p/rust-libp2p/tree/ce24938048cdb9c29f0b8218364557783cecd44a/misc/quick-protobuf-codec).
The original hidden generated test-message API is retained. Local focused
checks replace upstream test/benchmark harnesses so no test dependencies are
added to the runtime workspace.

The workspace patch adds an optional process-wide size observer at the actual
Protobuf encoder/decoder. The callback receives type, direction, completion
status, payload/framing sizes, advertised size, and DHT opcode metadata only.
It receives no payload, record key, address, public key, or secret. The
original encoding, acceptance limits, errors, and EOF behavior are retained.
`encoded` means serialized into a buffer, not sent on the socket. An oversized
advertised size is kept separate from available received bytes. EOF-incomplete
frames can be reported; unfinished streams ending in transport errors are
outside this codec's completion accounting.

Run `cargo test --locked -p quick-protobuf-codec -j 4` from the workspace.
