#!/usr/bin/env bash
# Run this ONCE inside the RunPod pod's web terminal.
# Template: ComfyUI-TBG-ETUR cu130 (Sage+Flash attention pre-installed).

set -euo pipefail

# ---- 1. Locate ComfyUI -------------------------------------------------------
if [ -n "${COMFY_ROOT:-}" ] && [ -d "$COMFY_ROOT" ]; then
  COMFY="$COMFY_ROOT"
elif [ -d "/workspace/ComfyUI" ]; then
  COMFY="/workspace/ComfyUI"
elif [ -d "/ComfyUI" ]; then
  COMFY="/ComfyUI"
else
  echo "Auto-locating ComfyUI..."
  FOUND=$(find / -maxdepth 5 -type d -name "ComfyUI" 2>/dev/null | head -1 || true)
  if [ -n "$FOUND" ]; then
    COMFY="$FOUND"
  else
    echo "ERROR: ComfyUI not found. Set COMFY_ROOT=/path/to/ComfyUI and rerun."
    exit 1
  fi
fi
echo "Using ComfyUI at: $COMFY"
MODELS="$COMFY/models"
mkdir -p "$MODELS"/{checkpoints,loras,controlnet}

# ---- 1b. Install aria2 for fast multi-connection downloads ------------------
if ! command -v aria2c >/dev/null 2>&1; then
  echo "==> Installing aria2..."
  apt-get update -qq && apt-get install -y -qq aria2
fi

# dl <url> <dest_dir> <filename>  — skip if file already present with nonzero size
dl() {
  local url="$1" dir="$2" name="$3"
  if [ -s "$dir/$name" ]; then
    echo "Already present: $dir/$name"
    return 0
  fi
  aria2c -x 16 -s 16 -k 1M --console-log-level=warn --summary-interval=5 \
    --file-allocation=none -d "$dir" -o "$name" "$url"
}

# ---- 2. SDXL Lightning LoRA -------------------------------------------------
echo "==> Downloading SDXL Lightning 8-step LoRA..."
dl "https://huggingface.co/ByteDance/SDXL-Lightning/resolve/main/sdxl_lightning_8step_lora.safetensors" \
   "$MODELS/loras" "sdxl_lightning_8step_lora.safetensors"

# ---- 3. ControlNet OpenPose for SDXL ----------------------------------------
echo "==> Downloading Xinsir OpenPose SDXL ControlNet (~2.5GB)..."
dl "https://huggingface.co/xinsir/controlnet-openpose-sdxl-1.0/resolve/main/diffusion_pytorch_model.safetensors" \
   "$MODELS/controlnet" "thibaud_openpose_sdxl.safetensors"

# ---- 4. Custom nodes ---------------------------------------------------------
CUSTOM="$COMFY/custom_nodes"
mkdir -p "$CUSTOM"
if [ ! -d "$CUSTOM/ComfyUI_IPAdapter_plus" ]; then
  echo "==> Installing ComfyUI_IPAdapter_plus custom node..."
  git clone https://github.com/cubiq/ComfyUI_IPAdapter_plus.git "$CUSTOM/ComfyUI_IPAdapter_plus"
fi

# ---- 5. DynaVision XL checkpoint from Civitai --------------------------------
CHECKPOINT="$MODELS/checkpoints/dynavisionXLAllInOneStylized_releaseV0610Bakedvae.safetensors"
CIVITAI_TOKEN="${CIVITAI_TOKEN:-c6dc51587319137aceabd6a4c8fe968f}"
CIVITAI_VERSION_ID="${CIVITAI_VERSION_ID:-297740}"

if [ ! -s "$CHECKPOINT" ]; then
  echo "==> Downloading DynaVision XL v0.610 from Civitai (~6.5 GB)..."
  wget -c --tries=5 --retry-connrefused --show-progress -O "$CHECKPOINT" \
    "https://civitai.com/api/download/models/${CIVITAI_VERSION_ID}?token=${CIVITAI_TOKEN}"
else
  echo "Checkpoint already present: $CHECKPOINT"
fi

# ---- 6. ComfyUI launch flags ------------------------------------------------
# Try common args file locations for different templates.
for ARGS_FILE in "/workspace/runpod-slim/comfyui_args.txt" "/workspace/comfyui_args.txt"; do
  if [ -d "$(dirname "$ARGS_FILE")" ]; then
    echo "==> Writing ComfyUI launch flags to $ARGS_FILE"
    printf "%s\n" "--highvram" "--fast" > "$ARGS_FILE"
    cat "$ARGS_FILE"
    break
  fi
done

echo
echo "============================================================"
echo "DONE. Restart ComfyUI in the pod (or restart the pod)."
echo "Then on your local machine, set in .env:"
echo "    COMFY_REMOTE_URL=https://<podid>-8188.proxy.runpod.net"
echo "============================================================"
