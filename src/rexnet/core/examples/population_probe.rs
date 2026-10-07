//! Local validation helper; starts reporting instances without loading a game.
use rexnet_core::population::Reporter;
use std::time::Duration;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    if args.len() < 4 {
        return Err("usage: population_probe <collector-origin> <seconds> <hex-title-id> [...]".into());
    }
    let seconds: u64 = args[2].parse()?;
    let mut reporters = Vec::new();
    for title in &args[3..] {
        reporters.push(Reporter::start(&args[1], u32::from_str_radix(title, 16)?)?);
    }
    std::thread::sleep(Duration::from_secs(seconds));
    drop(reporters);
    Ok(())
}
