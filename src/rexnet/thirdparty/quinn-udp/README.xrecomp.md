# Quinn UDP Wine/Proton compatibility backport

Source: crates.io `quinn-udp` 0.5.15, with the Windows patch and a local regression test.
The manifest isolates its test workspace and omits the unused Criterion benchmark dependency.
`Cargo.toml.orig` retains the original upstream manifest.
Upstream patch: https://github.com/quinn-rs/quinn/commit/e5b30f51341c323ca3fbaa4006912b105efc1018

The complete upstream Windows ECN fix is backported to the version used by
libp2p-quic 0.13.1. Unsupported IP_RECVECN/IPV6_RECVECN disables ECN for that
address family, including outgoing ancillary data. Other socket errors remain
fatal. The root Cargo patch keeps the existing Quinn/libp2p API and lock versions.
Remove this vendor override when a compatible upstream 0.5 release includes
this fix. Original MIT and Apache-2.0 licenses are included.

Local follow-up: null the WSAMSG control pointer when no ancillary messages
remain. The IPv4/IPv6 test found native Winsock otherwise returns WSAEFAULT
(10014) after ECN is disabled. The test injects unsupported ECN receive options,
checks other errors remain fatal, and verifies both send entry points deliver
unmarked packets to real loopback sockets.

Second Wine follow-up: query IPV6_V6ONLY only on IPv6 sockets. The tester's
fixed-build log still reports WSAEOPNOTSUPP (10045) for IPv4 QUIC startup;
the original unconditional IPv6 query ran before the ECN fallback. The
packet regression now also rejects that query on IPv4, proving setup skips
it while IPv6 and dual-stack behavior remain covered by the upstream tests.
