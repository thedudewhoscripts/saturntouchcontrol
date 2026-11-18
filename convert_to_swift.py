import os
import json
import urllib.request

MODEL = "qwen2.5-coder:7b"
OLLAMA_URL = "http://localhost:11434/api"

SOURCE_DIR = "."
OUTPUT_DIR = "swiftport"
SKIP_DIRS = {".git", "build", "external", "swiftport", "doxygen", "bin"}
VALID_EXTS = {".c", ".cpp", ".h", ".hpp"}
MAX_FILE_SIZE_BYTES = 100 * 1024


def ensure_model_downloaded(model_name):
    print(f"Checking/downloading AI model '{model_name}' via Ollama...")
    req = urllib.request.Request(
        f"{OLLAMA_URL}/pull",
        data=json.dumps({"name": model_name, "stream": True}).encode("utf-8"),
        headers={"Content-Type": "application/json"},
    )
    with urllib.request.urlopen(req) as resp:
        for line in resp:
            if line:
                data = json.loads(line.decode("utf-8"))
                status = data.get("status", "")
                if status:
                    print(f"  {status}          ", end="\r")
    print(f"\nModel '{model_name}' is ready!\n")


def translate_to_swift(code, rel_path):
    prompt = (
        f"You are an expert C, C++, and Swift systems programmer.\n"
        f"Convert the following file ({rel_path}) from C/C++ into clean, idiomatic Swift.\n"
        f"Rules:\n"
        f"1. Output ONLY valid Swift code. Do NOT include markdown fences (```swift) or explanations.\n"
        f"2. Preserve all function names, struct layouts, logic, and comments.\n"
        f"3. Convert C pointers, arrays, and macros into safe Swift equivalents.\n\n"
        f"SOURCE CODE:\n{code}"
    )

    payload = json.dumps({
        "model": MODEL,
        "prompt": prompt,
        "stream": False,
        "options": {"temperature": 0.1, "num_ctx": 8192},
    }).encode("utf-8")

    req = urllib.request.Request(
        f"{OLLAMA_URL}/generate",
        data=payload,
        headers={"Content-Type": "application/json"},
    )

    with urllib.request.urlopen(req, timeout=600) as resp:
        result = json.loads(resp.read().decode("utf-8")).get("response", "").strip()

    if result.startswith("```"):
        lines = result.splitlines()
        if lines[0].startswith("```"):
            lines = lines[1:]
        if lines and lines[-1].startswith("```"):
            lines = lines[:-1]
        result = "\n".join(lines)

    return result


def main():
    ensure_model_downloaded(MODEL)

    for root, dirs, files in os.walk(SOURCE_DIR):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]

        for filename in files:
            _, ext = os.path.splitext(filename)
            ext = ext.lower()
            if ext not in VALID_EXTS:
                continue

            src_path = os.path.join(root, filename)
            rel_path = os.path.relpath(src_path, SOURCE_DIR)

            base_no_ext, _ = os.path.splitext(rel_path)
            suffix = "_header.swift" if ext in {".h", ".hpp"} else ".swift"
            dest_path = os.path.join(OUTPUT_DIR, base_no_ext + suffix)

            if os.path.exists(dest_path):
                print(f"[SKIP - Exists] {rel_path}")
                continue

            if os.path.getsize(src_path) > MAX_FILE_SIZE_BYTES:
                print(f"[SKIP - Too Large] {rel_path}")
                continue

            os.makedirs(os.path.dirname(dest_path), exist_ok=True)
            print(f"[CONVERTING] {rel_path} -> {dest_path}")

            try:
                with open(src_path, "r", encoding="utf-8", errors="ignore") as f:
                    source_code = f.read()

                swift_code = translate_to_swift(source_code, rel_path)

                with open(dest_path, "w", encoding="utf-8") as out:
                    out.write(swift_code + "\n")

            except Exception as e:
                print(f"[ERROR] Failed on {rel_path}: {e}")


if __name__ == "__main__":
    main()
