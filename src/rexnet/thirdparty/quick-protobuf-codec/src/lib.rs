#![cfg_attr(docsrs, feature(doc_cfg, doc_auto_cfg))]

use asynchronous_codec::{Decoder, Encoder};
use bytes::{Buf, BufMut, BytesMut};
use measurement::{observe, Direction, Outcome};
use quick_protobuf::{BytesReader, MessageRead, MessageWrite, Writer, WriterBackend};
use std::io;
use std::marker::PhantomData;

mod generated;

pub mod measurement;

#[doc(hidden)] // NOT public API. Do not use.
pub use generated::test as proto;

/// [`Codec`] implements [`Encoder`] and [`Decoder`], uses [`unsigned_varint`]
/// to prefix messages with their length and uses [`quick_protobuf`] and a provided
/// `struct` implementing [`MessageRead`] and [`MessageWrite`] to do the encoding.
pub struct Codec<In, Out = In> {
    max_message_len_bytes: usize,
    phantom: PhantomData<(In, Out)>,
}

impl<In, Out> Codec<In, Out> {
    /// Create new [`Codec`].
    ///
    /// Parameter `max_message_len_bytes` determines the maximum length of the
    /// Protobuf message. The limit does not include the bytes needed for the
    /// [`unsigned_varint`].
    pub fn new(max_message_len_bytes: usize) -> Self {
        Self {
            max_message_len_bytes,
            phantom: PhantomData,
        }
    }
}

impl<In: MessageWrite, Out> Encoder for Codec<In, Out> {
    type Item<'a> = In;
    type Error = Error;

    fn encode(&mut self, item: Self::Item<'_>, dst: &mut BytesMut) -> Result<(), Self::Error> {
        let start = dst.len();
        let declared = item.get_size();
        write_length(&item, dst);
        let payload_start = dst.len();
        let result = write_message(&item, dst);
        observe::<In>(
            Direction::Encoded,
            if result.is_ok() {
                Outcome::Complete
            } else {
                Outcome::Incomplete
            },
            &dst[payload_start..],
            payload_start - start,
            Some(declared),
        );
        result?;

        Ok(())
    }
}

/// Write the message's length (i.e. `size`) to `dst` as a variable-length integer.
fn write_length(message: &impl MessageWrite, dst: &mut BytesMut) {
    let message_length = message.get_size();

    let mut uvi_buf = unsigned_varint::encode::usize_buffer();
    let encoded_length = unsigned_varint::encode::usize(message_length, &mut uvi_buf);

    dst.extend_from_slice(encoded_length);
}

/// Write the message itself to `dst`.
fn write_message(item: &impl MessageWrite, dst: &mut BytesMut) -> io::Result<()> {
    let mut writer = Writer::new(BytesMutWriterBackend::new(dst));
    item.write_message(&mut writer)
        .map_err(|e| io::Error::new(io::ErrorKind::Other, e))?;

    Ok(())
}

impl<In, Out> Decoder for Codec<In, Out>
where
    Out: for<'a> MessageRead<'a>,
{
    type Item = Out;
    type Error = Error;

    fn decode(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        let (message_length, remaining) = match unsigned_varint::decode::usize(src) {
            Ok((len, remaining)) => (len, remaining),
            Err(unsigned_varint::decode::Error::Insufficient) => return Ok(None),
            Err(e) => {
                observe::<Out>(
                    Direction::Received,
                    Outcome::Malformed,
                    &[],
                    src.len(),
                    None,
                );
                return Err(Error(io::Error::new(io::ErrorKind::InvalidData, e)));
            }
        };

        let varint_length = src.len() - remaining.len();
        if message_length > self.max_message_len_bytes {
            observe::<Out>(
                Direction::Received,
                Outcome::Oversized,
                &remaining[..remaining.len().min(message_length)],
                varint_length,
                Some(message_length),
            );
            return Err(Error(io::Error::new(
                io::ErrorKind::PermissionDenied,
                format!(
                    "message with {message_length}b exceeds maximum of {}b",
                    self.max_message_len_bytes
                ),
            )));
        }

        // Ensure we can read an entire message.
        if src.len() < (message_length + varint_length) {
            return Ok(None);
        }

        // Safe to advance buffer now.
        src.advance(varint_length);

        let message = src.split_to(message_length);

        let mut reader = BytesReader::from_bytes(&message);
        let result = Self::Item::from_reader(&mut reader, &message);
        observe::<Out>(
            Direction::Received,
            if result.is_ok() {
                Outcome::Complete
            } else {
                Outcome::Malformed
            },
            &message,
            varint_length,
            Some(message_length),
        );
        let message = result.map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

        Ok(Some(message))
    }

    fn decode_eof(&mut self, src: &mut BytesMut) -> Result<Option<Self::Item>, Self::Error> {
        let result = self.decode(src);
        if matches!(&result, Ok(None)) && !src.is_empty() {
            match unsigned_varint::decode::usize(src) {
                Ok((declared, remaining)) => observe::<Out>(
                    Direction::Received,
                    Outcome::Incomplete,
                    remaining,
                    src.len() - remaining.len(),
                    Some(declared),
                ),
                Err(_) => observe::<Out>(
                    Direction::Received,
                    Outcome::Incomplete,
                    &[],
                    src.len(),
                    None,
                ),
            }
        }
        // Preserve upstream EOF/error behavior exactly; observation never
        // changes a message, the acceptance limit, or decoder buffer ownership.
        result
    }
}

struct BytesMutWriterBackend<'a> {
    dst: &'a mut BytesMut,
}

impl<'a> BytesMutWriterBackend<'a> {
    fn new(dst: &'a mut BytesMut) -> Self {
        Self { dst }
    }
}

impl<'a> WriterBackend for BytesMutWriterBackend<'a> {
    fn pb_write_u8(&mut self, x: u8) -> quick_protobuf::Result<()> {
        self.dst.put_u8(x);

        Ok(())
    }

    fn pb_write_u32(&mut self, x: u32) -> quick_protobuf::Result<()> {
        self.dst.put_u32_le(x);

        Ok(())
    }

    fn pb_write_i32(&mut self, x: i32) -> quick_protobuf::Result<()> {
        self.dst.put_i32_le(x);

        Ok(())
    }

    fn pb_write_f32(&mut self, x: f32) -> quick_protobuf::Result<()> {
        self.dst.put_f32_le(x);

        Ok(())
    }

    fn pb_write_u64(&mut self, x: u64) -> quick_protobuf::Result<()> {
        self.dst.put_u64_le(x);

        Ok(())
    }

    fn pb_write_i64(&mut self, x: i64) -> quick_protobuf::Result<()> {
        self.dst.put_i64_le(x);

        Ok(())
    }

    fn pb_write_f64(&mut self, x: f64) -> quick_protobuf::Result<()> {
        self.dst.put_f64_le(x);

        Ok(())
    }

    fn pb_write_all(&mut self, buf: &[u8]) -> quick_protobuf::Result<()> {
        self.dst.put_slice(buf);

        Ok(())
    }
}

#[derive(thiserror::Error, Debug)]
#[error("Failed to encode/decode message")]
pub struct Error(#[from] io::Error);

impl From<Error> for io::Error {
    fn from(e: Error) -> Self {
        e.0
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::measurement::{Direction, Observation, Outcome};
    use std::sync::Mutex;

    static SEEN: Mutex<Vec<Observation>> = Mutex::new(Vec::new());

    #[test]
    fn sizes_follow_real_frames_and_never_count_partial_reads_as_messages() {
        assert!(measurement::install_observer(|row| SEEN
            .lock()
            .unwrap()
            .push(row)));
        let mut codec = Codec::<proto::Message>::new(16 * 1024);
        let original = proto::Message {
            data: vec![7; 2048],
        };
        let mut wire = BytesMut::new();
        codec.encode(original.clone(), &mut wire).unwrap();
        let payload = original.get_size();
        assert_eq!(wire.len(), payload + 2);
        assert_eq!(
            SEEN.lock().unwrap().len(),
            1,
            "successful encoding must be measured"
        );
        let mut partial = wire.split_to(1);
        assert!(codec.decode(&mut partial).unwrap().is_none());
        partial.extend_from_slice(&wire[..100]);
        assert!(codec.decode(&mut partial).unwrap().is_none());
        assert_eq!(
            SEEN.lock().unwrap().len(),
            1,
            "partial reads must not inflate counts"
        );
        partial.extend_from_slice(&wire[100..]);
        assert_eq!(codec.decode(&mut partial).unwrap().unwrap(), original);
        assert!(partial.is_empty());
        assert_eq!(SEEN.lock().unwrap().len(), 2);
        let rows = SEEN.lock().unwrap();
        assert_eq!(rows[0].direction, Direction::Encoded);
        assert_eq!(rows[1].direction, Direction::Received);
        for row in rows.iter() {
            assert_eq!(row.outcome, Outcome::Complete);
            assert_eq!(row.payload_bytes, payload);
            assert_eq!(row.prefix_bytes, 2);
            assert_eq!(row.declared_payload_bytes, Some(payload));
        }
        drop(rows);

        // A declared 16,385-byte frame is rejected after its three-byte header.
        let mut oversized = BytesMut::from(&[0x81, 0x80, 0x01][..]);
        assert!(codec.decode(&mut oversized).is_err());
        assert_eq!(
            oversized.len(),
            3,
            "observation must preserve decoder behavior"
        );
        let rejected = *SEEN.lock().unwrap().last().unwrap();
        assert_eq!(rejected.outcome, Outcome::Oversized);
        assert_eq!(rejected.declared_payload_bytes, Some(16_385));
        assert_eq!(
            rejected.payload_bytes, 0,
            "declared bytes are not received bytes"
        );
        assert_eq!(rejected.prefix_bytes, 3);

        // A full malformed Protobuf and an EOF with an unfinished frame.
        assert!(codec.decode(&mut BytesMut::from(&[1, 0x0f][..])).is_err());
        assert_eq!(
            SEEN.lock().unwrap().last().unwrap().outcome,
            Outcome::Malformed
        );
        let mut unfinished = BytesMut::from(&[5, 0x0a][..]);
        assert!(codec.decode_eof(&mut unfinished).unwrap().is_none());
        assert_eq!(unfinished.len(), 2);
        let incomplete = *SEEN.lock().unwrap().last().unwrap();
        assert_eq!(incomplete.outcome, Outcome::Incomplete);
        assert_eq!(incomplete.payload_bytes, 1);
        assert_eq!(incomplete.declared_payload_bytes, Some(5));
        assert!(codec.decode(&mut BytesMut::from(&[0x80; 12][..])).is_err());
        assert_eq!(
            SEEN.lock().unwrap().last().unwrap().outcome,
            Outcome::Malformed
        );

        // Coalesced frames and an empty payload keep their individual lengths.
        let mut coalesced = BytesMut::new();
        codec
            .encode(proto::Message::default(), &mut coalesced)
            .unwrap();
        codec
            .encode(proto::Message { data: vec![9; 100] }, &mut coalesced)
            .unwrap();
        assert!(codec
            .decode(&mut coalesced)
            .unwrap()
            .unwrap()
            .data
            .is_empty());
        assert_eq!(SEEN.lock().unwrap().last().unwrap().payload_bytes, 0);
        assert_eq!(
            codec.decode(&mut coalesced).unwrap().unwrap().data.len(),
            100
        );
        assert_eq!(SEEN.lock().unwrap().last().unwrap().payload_bytes, 102);
        assert!(coalesced.is_empty());

        // The upstream limit is receive-only. Observation must not add an
        // outbound cap or silently truncate a message.
        let mut large = BytesMut::new();
        codec
            .encode(
                proto::Message {
                    data: vec![1; 20_000],
                },
                &mut large,
            )
            .unwrap();
        assert_eq!(SEEN.lock().unwrap().last().unwrap().payload_bytes, 20_004);
        assert!(codec.decode(&mut large).is_err());
    }
}
