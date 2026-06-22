import { existsSync, readFileSync, writeFileSync } from "fs";
import { createHash } from "crypto";

interface StopHookInput {
  conversation_id: string;
  status: "completed" | "aborted" | "error";
  loop_count: number;
}

const SCRATCHPAD = ".cursor/ida-rename-scratchpad.md";
const STATE_FILE = ".cursor/ida-rename-loop-state.json";

const MAX_ITERATIONS = Number(process.env.IDA_RENAME_MAX_ITERATIONS ?? 80);
const MAX_NO_PROGRESS_LOOPS = Number(process.env.IDA_RENAME_MAX_NO_PROGRESS_LOOPS ?? 2);
const BATCH_SIZE = Number(process.env.IDA_RENAME_BATCH_SIZE ?? 12);

function readText(path: string): string {
  return existsSync(path) ? readFileSync(path, "utf8") : "";
}

function hashText(text: string): string {
  return createHash("sha256").update(text).digest("hex");
}

function exitNoFollowup() {
  console.log(JSON.stringify({}));
  process.exit(0);
}

const input = (await Bun.stdin.json()) as StopHookInput;

if (input.status !== "completed") {
  exitNoFollowup();
}

if (input.loop_count >= MAX_ITERATIONS) {
  exitNoFollowup();
}

const scratchpad = readText(SCRATCHPAD);

if (!scratchpad.includes("IDA_RENAME_LOOP: active")) {
  exitNoFollowup();
}

if (scratchpad.includes("IDA_RENAME_LOOP: done")) {
  exitNoFollowup();
}

type LoopState = {
  lastScratchpadHash?: string;
  noProgressLoops?: number;
};

let state: LoopState = {};

try {
  state = JSON.parse(readText(STATE_FILE) || "{}");
} catch {
  state = {};
}

const currentHash = hashText(scratchpad);

if (state.lastScratchpadHash === currentHash) {
  state.noProgressLoops = (state.noProgressLoops ?? 0) + 1;
} else {
  state.noProgressLoops = 0;
}

state.lastScratchpadHash = currentHash;

writeFileSync(STATE_FILE, JSON.stringify(state, null, 2));

if ((state.noProgressLoops ?? 0) >= MAX_NO_PROGRESS_LOOPS) {
  exitNoFollowup();
}

const followupMessage = `
Continue the IDA function-renaming loop.

You are working on reverse engineering in IDA. Your job is to rename the next batch of still-unnamed functions.

Use the available IDA MCP tools, IDAPython tools, or existing project scripts. Do not invent names without evidence.

Hard rules:

1. Work on at most ${BATCH_SIZE} candidate functions this iteration.
2. Treat names like sub_*, nullsub_*, j_*, FUN_*, Function_*, or auto-generated placeholder names as rename candidates.
3. Do not overwrite meaningful user-created names.
4. For each candidate, inspect enough evidence before renaming:
   - decompiler output
   - callers
   - callees
   - strings
   - imports
   - xrefs
   - constants
   - global data accesses
   - switch tables or vtables when relevant
5. Rename only when confidence is reasonably high.
6. Use IDA-safe snake_case names.
7. Prefer specific behavior names over vague names.
   Good:
   - parse_packet_header
   - update_audio_stream_state
   - alloc_stream_buffer
   - check_license_flags

   Bad:
   - do_stuff
   - handler
   - process_data
   - function_1
8. If unsure, skip the function and record why.
9. After each iteration, update ${SCRATCHPAD}.
10. If there are no more useful rename candidates, replace:

   IDA_RENAME_LOOP: active

   with:

   IDA_RENAME_LOOP: done

Required scratchpad update format:

Append a section like this:

## Iteration ${input.loop_count + 1}

| Address | Old Name | New Name | Confidence | Evidence |
|---|---|---|---:|---|
| 0x12345678 | sub_12345678 | parse_foo_header | 0.82 | References string "...", called before buffer decode, validates size fields |

Also add skipped functions:

| Address | Old Name | Reason Skipped |
|---|---|---|
| 0x12345678 | sub_12345678 | Only wrapper/thunk, not enough evidence |

Continue from the scratchpad. Do not redo functions already handled.
`;

console.log(
  JSON.stringify({
    followup_message: followupMessage
  })
);