// Flat, unlit placeholder pixel shader for draws issued from FM2's recorded
// command-buffer object-pass path (car/showroom geometry) -- see
// render_state.h's IsInsideRecordedBatch() comment. Deliberately reads no
// resources or constants: the real pixel shader's constants aren't reliably
// this draw's own by the time such a draw executes under this renderer.
float4 main() : SV_Target
{
    return float4(0.5, 0.5, 0.55, 1.0);
}
