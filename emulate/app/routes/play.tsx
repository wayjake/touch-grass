import { useEffect, useRef, useCallback } from "react";
import type { Route } from "./+types/play";

export function meta({}: Route.MetaArgs) {
  return [
    { title: "Touch Grass" },
    { name: "description", content: "A tile-based exploration game" },
  ];
}

// Button indices matching platform.h
const BTN_UP = 0;
const BTN_DOWN = 1;
const BTN_LEFT = 2;
const BTN_RIGHT = 3;
const BTN_A = 4;
const BTN_B = 5;

// Display constants
const SCREEN_WIDTH = 128;
const SCREEN_HEIGHT = 64;
const SCALE = 4;

type GameModule = {
  _game_init: () => void;
  _game_tick: () => void;
  _platform_get_framebuffer: () => number;
  _platform_get_framebuffer_size: () => number;
  _platform_set_button: (btn: number, pressed: number) => void;
  HEAPU8?: Uint8Array;
  wasmMemory?: WebAssembly.Memory;
};

export default function Play() {
  const canvasRef = useRef<HTMLCanvasElement>(null);
  const moduleRef = useRef<GameModule | null>(null);
  const animationRef = useRef<number>(0);

  const renderFrame = useCallback(() => {
    const module = moduleRef.current;
    const canvas = canvasRef.current;
    if (!module || !canvas || !module.HEAPU8) return;

    const ctx = canvas.getContext("2d");
    if (!ctx) return;

    // Get framebuffer from WASM memory
    const fbPtr = module._platform_get_framebuffer();
    const fbSize = module._platform_get_framebuffer_size();
    if (!fbPtr || !fbSize) return;
    const framebuffer = module.HEAPU8.subarray(fbPtr, fbPtr + fbSize);

    // Create ImageData for the scaled canvas
    const imageData = ctx.createImageData(SCREEN_WIDTH * SCALE, SCREEN_HEIGHT * SCALE);
    const pixels = imageData.data;

    // Decode framebuffer and render with scaling
    for (let y = 0; y < SCREEN_HEIGHT; y++) {
      for (let x = 0; x < SCREEN_WIDTH; x++) {
        const bitIndex = y * SCREEN_WIDTH + x;
        const byteIndex = Math.floor(bitIndex / 8);
        const bitOffset = bitIndex % 8;
        const isSet = (framebuffer[byteIndex] >> bitOffset) & 1;

        const color = isSet ? 255 : 0;

        // Draw scaled pixel
        for (let sy = 0; sy < SCALE; sy++) {
          for (let sx = 0; sx < SCALE; sx++) {
            const px = x * SCALE + sx;
            const py = y * SCALE + sy;
            const idx = (py * SCREEN_WIDTH * SCALE + px) * 4;
            pixels[idx] = color;     // R
            pixels[idx + 1] = color; // G
            pixels[idx + 2] = color; // B
            pixels[idx + 3] = 255;   // A
          }
        }
      }
    }

    ctx.putImageData(imageData, 0, 0);
  }, []);

  const gameLoop = useCallback(() => {
    const module = moduleRef.current;
    if (module) {
      module._game_tick();
      renderFrame();
    }
    animationRef.current = requestAnimationFrame(gameLoop);
  }, [renderFrame]);

  useEffect(() => {
    let mounted = true;

    async function init() {
      // Load the WASM module via script tag (required for public folder assets)
      await new Promise<void>((resolve, reject) => {
        const script = document.createElement("script");
        script.src = "/wasm/game.js";
        script.onload = () => resolve();
        script.onerror = reject;
        document.head.appendChild(script);
      });

      // createGameModule is now available globally
      const createGameModule = (window as unknown as { createGameModule: () => Promise<GameModule> }).createGameModule;
      const module = await createGameModule();

      if (!mounted) return;

      moduleRef.current = module;

      // Initialize game
      module._game_init();

      // Debug: check module state
      console.log("Module keys:", Object.keys(module));
      console.log("Module:", module);

      // Start game loop
      animationRef.current = requestAnimationFrame(gameLoop);
    }

    init();

    return () => {
      mounted = false;
      if (animationRef.current) {
        cancelAnimationFrame(animationRef.current);
      }
    };
  }, [gameLoop]);

  useEffect(() => {
    const keyMap: Record<string, number> = {
      ArrowUp: BTN_UP,
      ArrowDown: BTN_DOWN,
      ArrowLeft: BTN_LEFT,
      ArrowRight: BTN_RIGHT,
      KeyZ: BTN_A,
      KeyX: BTN_B,
      Space: BTN_A,
      Enter: BTN_A,
      Escape: BTN_B,
    };

    function handleKeyDown(e: KeyboardEvent) {
      const btn = keyMap[e.code];
      if (btn !== undefined && moduleRef.current) {
        e.preventDefault();
        moduleRef.current._platform_set_button(btn, 1);
      }
    }

    function handleKeyUp(e: KeyboardEvent) {
      const btn = keyMap[e.code];
      if (btn !== undefined && moduleRef.current) {
        e.preventDefault();
        moduleRef.current._platform_set_button(btn, 0);
      }
    }

    window.addEventListener("keydown", handleKeyDown);
    window.addEventListener("keyup", handleKeyUp);

    return () => {
      window.removeEventListener("keydown", handleKeyDown);
      window.removeEventListener("keyup", handleKeyUp);
    };
  }, []);

  return (
    <div className="flex flex-col items-center justify-center min-h-screen bg-gray-900">
      <h1 className="text-2xl font-bold text-white mb-4">Touch Grass</h1>
      <canvas
        ref={canvasRef}
        width={SCREEN_WIDTH * SCALE}
        height={SCREEN_HEIGHT * SCALE}
        className="border-4 border-gray-700 rounded"
        style={{ imageRendering: "pixelated" }}
      />
      <div className="mt-4 text-gray-400 text-sm">
        <p>Arrow keys: Move | Z/Space: A button | X/Esc: B button</p>
      </div>
    </div>
  );
}
