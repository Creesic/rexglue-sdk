//! Anonymous, optional population reporting; independent of the libp2p engine.

use rand::RngCore;
use reqwest::{redirect::Policy, Url};
use serde::Serialize;
use std::net::IpAddr;
use std::sync::mpsc::{self, Receiver, Sender};
use std::time::{Duration, Instant};

const INTERVAL: Duration = Duration::from_secs(30);
const REQUEST_TIMEOUT: Duration = Duration::from_secs(5);
const STOP_TIMEOUT: Duration = Duration::from_millis(100);

fn endpoint(origin: &str) -> Result<Url, &'static str> {
    // Check the raw origin too: URL parsing normalizes /a/.. and legacy numeric IPs.
    let (scheme, rest) = origin.split_once("://").ok_or("invalid population origin")?;
    if origin.chars().any(|c| c.is_control() || c.is_whitespace())
        || rest.contains(['\\', '?', '#', '@'])
        || rest.split_once('/').is_some_and(|(_, path)| !path.is_empty())
    {
        return Err("population URL must be an origin without credentials or a path");
    }
    let mut url = Url::parse(origin).map_err(|_| "invalid population origin")?;
    if url.host_str().is_none() || !url.username().is_empty() || url.password().is_some() {
        return Err("invalid population origin");
    }
    if scheme == "http" {
        let authority = rest.trim_end_matches('/');
        let raw_host = if let Some(ipv6) = authority.strip_prefix('[') {
            ipv6.split_once(']').map(|(host, _)| host)
        } else {
            Some(authority.split(':').next().unwrap_or_default())
        };
        if !raw_host.and_then(|host| host.parse::<IpAddr>().ok()).is_some_and(|ip| ip.is_loopback()) {
            return Err("HTTP population reporting is restricted to numeric loopback");
        }
    } else if scheme != "https" {
        return Err("remote population reporting requires HTTPS");
    }
    url.set_path("/v1/heartbeat");
    Ok(url)
}

#[derive(Serialize)]
struct Heartbeat {
    schema: u8,
    instance_id: String,
    title_id: String,
    seq: u64,
}

/// Owned independent worker. Dropping it signals stop and waits at most 100 ms.
/// An in-flight HTTP request may finish in the background (five-second timeout).
pub struct Reporter {
    stop: Sender<()>,
    done: Receiver<()>,
}

impl Reporter {
    pub fn from_config(origin: Option<&str>, title: u32) -> Result<Option<Self>, &'static str> {
        match origin {
            None | Some("") => Ok(None),
            Some(origin) => Self::start(origin, title).map(Some),
        }
    }

    pub fn start(origin: &str, title: u32) -> Result<Self, &'static str> {
        Self::start_with_interval(origin, title, INTERVAL)
    }

    fn start_with_interval(origin: &str, title: u32, interval: Duration) -> Result<Self, &'static str> {
        if title == 0 {
            return Err("invalid population title ID");
        }
        let url = endpoint(origin)?;
        let mut random = [0u8; 16];
        rand::rngs::OsRng.try_fill_bytes(&mut random).map_err(|_| "population randomness unavailable")?;
        let heartbeat = Heartbeat {
            schema: 1,
            instance_id: random.iter().map(|b| format!("{b:02x}")).collect(),
            title_id: format!("{title:08x}"),
            seq: 1,
        };
        let (stop, stopped) = mpsc::channel();
        let (finished, done) = mpsc::channel();
        std::thread::Builder::new().name("rexnet-population".into()).spawn(move || {
            run(url, heartbeat, stopped, interval);
            let _ = finished.send(());
        }).map_err(|_| "population worker could not start")?;
        Ok(Self { stop, done })
    }
}

impl Drop for Reporter {
    fn drop(&mut self) {
        let _ = self.stop.send(());
        let _ = self.done.recv_timeout(STOP_TIMEOUT);
    }
}

fn run(url: Url, mut heartbeat: Heartbeat, stopped: Receiver<()>, interval: Duration) {
    let client = match reqwest::blocking::Client::builder()
        .timeout(REQUEST_TIMEOUT).connect_timeout(REQUEST_TIMEOUT)
        .redirect(Policy::none()).no_proxy().build()
    {
        Ok(client) => client,
        Err(_) => {
            eprintln!("[POPULATION] HTTP client unavailable; reporting disabled");
            return;
        }
    };
    let mut next = Instant::now();
    let mut healthy = None;
    loop {
        if !matches!(stopped.try_recv(), Err(mpsc::TryRecvError::Empty)) {
            break;
        }
        let body = match serde_json::to_vec(&heartbeat) {
            Ok(body) => body,
            Err(_) => break,
        };
        let ok = client.post(url.clone()).header("Content-Type", "application/json")
            .body(body).send().is_ok_and(|response| response.status().as_u16() == 204);
        if healthy != Some(ok) {
            eprintln!("[POPULATION] {}", if ok { "reporting active" } else { "collector unavailable; gameplay unaffected" });
            healthy = Some(ok);
        }
        let Some(seq) = heartbeat.seq.checked_add(1) else { break; };
        heartbeat.seq = seq;
        let now = Instant::now();
        next += interval;
        while next <= now {
            next += interval;
        }
        match stopped.recv_timeout(next.saturating_duration_since(Instant::now())) {
            Err(mpsc::RecvTimeoutError::Timeout) => {},
            _ => break,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::{Read, Write};
    use std::net::TcpListener;

    #[test]
    fn origins_are_restricted_and_do_not_normalize_away_paths() {
        for good in ["https://example.org", "https://example.org:8443/", "http://127.0.0.1:8765", "http://[::1]:8765/"] {
            assert!(endpoint(good).is_ok(), "{good}");
        }
        for bad in ["", "http://example.org", "http://localhost", "http://127.0.0.1.evil.test",
            "http://2130706433", "http://127.1", "http://0x7f000001", "https://u:p@example.org",
            "https://example.org?q=x", "https://example.org#x", "https://example.org/a/..",
            "https://example.org/./", "https://example.org\\x", "https://example.org/v1/heartbeat"] {
            assert!(endpoint(bad).is_err(), "{bad}");
        }
    }

    #[test]
    fn sends_immediately_refreshes_and_stops_with_private_wire_shape() {
        let listener = TcpListener::bind("127.0.0.1:0").unwrap();
        let origin = format!("http://{}", listener.local_addr().unwrap());
        let (tx, rx) = std::sync::mpsc::channel();
        let server = std::thread::spawn(move || {
            for _ in 0..2 {
                let (mut socket, _) = listener.accept().unwrap();
                socket.set_read_timeout(Some(Duration::from_secs(2))).unwrap();
                let mut bytes = Vec::new();
                let mut byte = [0];
                while !bytes.ends_with(b"\r\n\r\n") {
                    socket.read_exact(&mut byte).unwrap();
                    bytes.push(byte[0]);
                }
                let headers = String::from_utf8(bytes).unwrap();
                assert!(headers.starts_with("POST /v1/heartbeat HTTP/1.1\r\n"));
                let len = headers.lines().find_map(|l| l.to_ascii_lowercase().strip_prefix("content-length:").map(|v| v.trim().parse::<usize>().unwrap())).unwrap();
                let mut body = vec![0; len];
                socket.read_exact(&mut body).unwrap();
                tx.send(serde_json::from_slice::<serde_json::Value>(&body).unwrap()).unwrap();
                socket.write_all(b"HTTP/1.1 204 No Content\r\nContent-Length: 0\r\nConnection: close\r\n\r\n").unwrap();
            }
        });
        let reporter = Reporter::start_with_interval(&origin, 0x5454_0079, Duration::from_millis(50)).unwrap();
        let first = rx.recv_timeout(Duration::from_secs(3)).unwrap();
        let second = rx.recv_timeout(Duration::from_secs(3)).unwrap();
        assert_eq!(first.as_object().unwrap().len(), 4);
        assert_eq!(first["schema"], 1);
        assert_eq!(first["title_id"], "54540079");
        assert_eq!(first["seq"], 1);
        assert_eq!(second["seq"], 2);
        assert_eq!(first["instance_id"], second["instance_id"]);
        let id = first["instance_id"].as_str().unwrap();
        assert_eq!(id.len(), 32);
        assert!(id.bytes().all(|c| c.is_ascii_hexdigit() && !c.is_ascii_uppercase()));
        let start = Instant::now();
        drop(reporter);
        assert!(start.elapsed() < Duration::from_secs(1));
        server.join().unwrap();
    }

    #[test]
    fn refused_collector_and_disabled_config_never_block_caller() {
        assert!(Reporter::from_config(None, 1).unwrap().is_none());
        assert!(Reporter::from_config(Some(""), 1).unwrap().is_none());
        assert!(Reporter::start("http://127.0.0.1:1", 0).is_err());
        let start = Instant::now();
        let reporter = Reporter::start("http://127.0.0.1:1", 1).unwrap();
        assert!(start.elapsed() < Duration::from_secs(1));
        drop(reporter);
        assert!(start.elapsed() < Duration::from_secs(1));
    }

    #[test]
    fn stalled_response_and_redirect_do_not_block_drop_or_follow() {
        let redirect_target = TcpListener::bind("127.0.0.1:0").unwrap();
        redirect_target.set_nonblocking(true).unwrap();
        let redirect = format!("HTTP/1.1 307 Temporary Redirect\r\nLocation: http://{}\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", redirect_target.local_addr().unwrap());
        for response in [None, Some(redirect)] {
            let redirecting = response.is_some();
            let listener = TcpListener::bind("127.0.0.1:0").unwrap();
            let origin = format!("http://{}", listener.local_addr().unwrap());
            let (tx, rx) = std::sync::mpsc::channel();
            let server = std::thread::spawn(move || {
                let (mut socket, _) = listener.accept().unwrap();
                socket.set_read_timeout(Some(Duration::from_secs(2))).unwrap();
                let mut buffer = [0; 1024];
                assert!(socket.read(&mut buffer).unwrap() > 0);
                if let Some(response) = response {
                    socket.write_all(response.as_bytes()).unwrap();
                    tx.send(()).unwrap();
                } else {
                    tx.send(()).unwrap();
                    std::thread::sleep(Duration::from_millis(400));
                }
            });
            let reporter = Reporter::start(&origin, 1).unwrap();
            rx.recv_timeout(Duration::from_secs(3)).unwrap();
            if redirecting {
                std::thread::sleep(Duration::from_millis(200));
                assert!(matches!(redirect_target.accept(), Err(e) if e.kind() == std::io::ErrorKind::WouldBlock));
            }
            let start = Instant::now();
            drop(reporter);
            assert!(start.elapsed() < Duration::from_millis(300));
            server.join().unwrap();
        }
    }
}
