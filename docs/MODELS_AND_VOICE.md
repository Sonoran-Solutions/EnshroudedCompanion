# Models and voice

Status: proposed provider design and evaluation plan. No model has been benchmarked for this project. Research baseline: 2026-10-22.

## Recommended starting point

Use one modest local chat model for both dialogue and typed high-level intent, with deterministic game controllers beneath it. Implement an equivalent hosted text path through OpenRouter. Let the owner choose local-only or an explicit hosted configuration.

Do not select a model before proving that the companion has usable game observations and actions. A more capable model cannot compensate for a missing navigation or construction interface.

## Provider interfaces

Separate DialogueProvider and IntentProvider logically:

- Dialogue input: character/persona, bounded conversation, known context, and approved relevant memories. Output: text plus usage/error metadata.
- Intent input: goal, snapshot identity, current capability/schema version, allowed actions and relevant state. Output: a typed proposal or no action.
- Provider configuration: endpoint, configured model, returned model/provider when available, timeout, context limit, output limit, concurrency, route policy and capability flags.
- Errors: timeout, unavailable, rate-limited, refusal, context overflow, malformed/empty output, unsupported feature and unknown usage. All are explicit states.

Use a deterministic fake provider for tests. No tests should rely on a model repeating an exact sentence. Persist prompt/template versions and request IDs for reproducibility while respecting logging privacy.

"OpenAI-compatible" describes an interface family, not identical tool semantics. Check structured-output, tool choice and streaming behavior for each model/server/provider combination. Fail visibly if required capabilities are absent. A validated JSON-text fallback is acceptable only if explicitly designed and tested; never silently accept prose as a command.

## Local options

| Option | Intended role | What must be measured |
| --- | --- | --- |
| Small Qwen instruct/chat model | Initial dialogue and typed intent candidate | Instruction adherence, character consistency, latency, JSON/action validity |
| Small Gemma instruct/chat model | Alternative style/quality candidate | Same tests; tool/schema support depends on serving route |
| llama.cpp server | Flexible local inference with structured outputs | Correct chat template, quantization, GPU offload and context configuration |
| Ollama or LM Studio | Convenient model management and local endpoints | Exact endpoint feature support; no assumption of full API parity |

Representative published artifacts illustrate scale, not a final recommendation: Qwen3 4B Q4 is roughly 2.6 GB and 8B Q4 roughly 5.2 GB; Gemma 3 4B Q4 is roughly 3.4 GB and 12B Q4 roughly 8.2 GB. Check the exact tag and quantization when downloading ([Qwen registry](https://registry.ollama.com/library/qwen3/tags), [Gemma registry](https://registry.ollama.com/library/gemma3/tags)).

Those are **artifact sizes, not total VRAM requirements**. Context/KV cache, runtime buffers, vision, speech services and game rendering add memory. On an RTX 3080-class 10/12 GB machine, start by testing a 4–8B quantized text model with a bounded context, especially if the GPU also renders a game client. Do not assume a 12B model fits comfortably because its weights are smaller than nominal VRAM.

A separate inference machine can reduce contention, but adds transport/security and latency considerations. Linux inference does not prove a Windows-native mod will load under Wine/Proton; test these as separate compatibility axes.

Primary server references: [llama.cpp server](https://github.com/ggml-org/llama.cpp/blob/master/tools/server/README.md), [Ollama compatibility documentation](https://github.com/ollama/ollama/blob/main/docs/api/openai-compatibility.mdx).

## OpenRouter

Use the documented chat-completion interface and inspect available model/provider parameters rather than assuming every route accepts tools or structured responses. Pin or record routing choices where possible; routing changes can change quality, latency and data handling.

API keys belong in an environment variable or local secret configuration excluded from Git, not in game assets, shared saves, screenshots or logs. Only the backend should need the key. Use limited/revocable keys where available.

Hosted mode must disclose what leaves the machine: selected game context, relevant dialogue/memories and, only if enabled, voice-derived text or images. Local-only mode must not silently fall back to OpenRouter. Provider-specific retention and routing preferences need an explicit owner choice.

Sources: [chat API](https://openrouter.ai/docs/api/api-reference/chat/create-a-chat-completion), [model properties](https://openrouter.ai/docs/api/api-reference/models/list-all-models-and-their-properties), [pricing](https://openrouter.ai/pricing).

## Jev / System One

Treat Jev as an **optional typed decision-policy experiment**, not the roleplay model. Its published approach selects structured decisions instead of generating conversational text. A typed output does not prove the decision is tactically correct or safe.

LM Studio documents a System One-compatible endpoint requiring a compatible loaded decision model. This is not proof that official Jev weights can be downloaded and run locally. Verify model access, license, runtime and endpoint independently before scheduling work.

Compare any such selector against the deterministic baseline and the existing LLM intent provider on the same recorded scenarios. Keep dialogue separate. Do not place this experiment on the critical path.

Sources: [Typesafe introduction](https://typesafe.ai/blog/introducing-system-one-models-and-jev), [LM Studio endpoint documentation](https://lmstudio.ai/docs/developer/jev-compat/systemone).

## Voice

Text is the baseline. Add voice as a replaceable pipeline: push-to-talk -> speech recognition -> confirmed/parsed request -> dialogue -> speech synthesis.

Local candidates include [whisper.cpp](https://github.com/ggml-org/whisper.cpp) for recognition and [Piper](https://github.com/OHF-Voice/piper1-gpl) for synthesis. Audit software and individual voice-model licenses before bundling. Voice is optional; it must not force hosted processing.

Test interruption, echo/feedback, partial recognition, background speech and cancelled playback. The companion must stop speaking when interrupted without accidentally repeating an action. Recognition alone must not approve irreversible changes; use an explicit owner confirmation control for construction or inventory permissions.

Do not require cloned voices. If custom voice assets are added, establish authorization and license suitability.

## Costs and budgets

Cost depends on actual token counts and selected provider prices. Keep a usage ledger with known input/output/reasoning charges where exposed. Missing usage is UNKNOWN, not zero. Apply request, token, concurrency and session-spend limits, including conservative headroom for in-flight calls.

Illustrative calculation, **not a current price quote or forecast**:

- Three hours at two calls per minute = 360 requests.
- At 2,000 input and 200 output tokens each: 720,000 input + 72,000 output tokens.
- At hypothetical rates of $1 per million input and $3 per million output: $0.72 + $0.216 = **$0.936**, about $0.94.
- One call per second at the same request size costs 30 times as much.
- Voice, additional reasoning tokens, retries, credit-purchase fees and other services are excluded.

Control cost by using event-driven context, summarized history, short decisions and deterministic controllers. Do not call an LLM every game frame.

## Evaluation gate

Use the same fixture set and task schema for local and hosted providers:

1. Valid actions and correct no-action decisions.
2. Refusal of unsupported, stale, foreign-world and unauthorized proposals by the executor.
3. Dialogue grounding, persona consistency, quest scope and no-spoiler behavior.
4. Time to first dialogue token, complete decision latency and end-to-end action latency.
5. Game frame-time impact, VRAM/RAM, sustained thermal/compute load and concurrent voice impact.
6. Token usage, spend estimate, queue depth and timeout/rate-limit behavior.
7. Outage, cancellation, disconnect, reload and provider-switch behavior.

Record hardware, model ID/digest, quantization, context, serving version, templates, game load and sample counts. Report distributions rather than one favorable response. Performance targets should be ratified after P0/P1 measurements; no tokens-per-second or latency claim is validated yet.
