package net.runelite.client.plugins.gpushared.shim;

import java.lang.invoke.VarHandle;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import net.runelite.api.Texture;
import net.runelite.api.TextureProvider;

/**
 * Writes all 256 RS textures (128×128 RGBA each) into the "URRL_Textures"
 * shared memory region once, so Unreal Engine can create a Texture2DArray.
 *
 * Layout (~16 MB):
 *   [0]  ready_flag (uint32) — 0=not ready, 1=all textures written
 *   [4]  pad[4]
 *   [8]  texture[0..255]: each TEXTURE_SIZE*TEXTURE_SIZE*4 bytes RGBA
 */
public class TextureBridge
{
    public static final int TEXTURE_COUNT      = 256;
    public static final int TEXTURE_SIZE       = 128;

    private static final int BYTES_PER_TEXTURE = TEXTURE_SIZE * TEXTURE_SIZE * 4;
    private static final int OFF_READY         = 0;
    private static final int DATA_BASE         = 8;
    public  static final int TOTAL_BYTES       = DATA_BASE + TEXTURE_COUNT * BYTES_PER_TEXTURE;

    // ── JNI ──────────────────────────────────────────────────────────────────
    public native long       openTextureMemory(String name);
    public native void       closeTextureMemory(long handle);
    public native ByteBuffer mapTextureBuffer(long handle);

    // ── State ─────────────────────────────────────────────────────────────────
    private long       nativeHandle = 0;
    private ByteBuffer buf;

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    public void init(String name)
    {
        nativeHandle = openTextureMemory(name);
        if (nativeHandle == 0)
            throw new RuntimeException("Failed to open texture shared memory: " + name);
        buf = mapTextureBuffer(nativeHandle).order(ByteOrder.LITTLE_ENDIAN);
        buf.putInt(OFF_READY, 0); // clear ready flag
    }

    public void shutdown()
    {
        if (nativeHandle != 0)
        {
            closeTextureMemory(nativeHandle);
            nativeHandle = 0;
        }
    }

    /**
     * Convert and write all 256 RS textures into shared memory, then set the
     * ready flag so UE can create its Texture2DArray.
     * Call once on the client thread after all textures are loaded.
     */
    public void sendTextures(TextureProvider textureProvider)
    {
        if (buf == null) return;

        Texture[] textures = textureProvider.getTextures();
        double savedBrightness = textureProvider.getBrightness();
        textureProvider.setBrightness(1.0);

        for (int id = 0; id < TEXTURE_COUNT; id++)
        {
            int base = DATA_BASE + id * BYTES_PER_TEXTURE;

            if (id < textures.length && textures[id] != null)
            {
                int[] pixels = textureProvider.load(id);
                if (pixels != null && pixels.length == TEXTURE_SIZE * TEXTURE_SIZE)
                {
                    // RS pixels are 0x00RRGGBB — alpha byte is always 0.
                    // Match the GL plugin: rgb==0 means transparent (A=0), otherwise opaque (A=255).
                    for (int i = 0; i < pixels.length; i++)
                    {
                        int argb = pixels[i];
                        buf.put(base + i * 4 + 0, (byte) ((argb >> 16) & 0xFF)); // R
                        buf.put(base + i * 4 + 1, (byte) ((argb >>  8) & 0xFF)); // G
                        buf.put(base + i * 4 + 2, (byte) ( argb        & 0xFF)); // B
                        buf.put(base + i * 4 + 3, (argb & 0x00FFFFFF) != 0 ? (byte) -1 : (byte) 0); // A
                    }
                    continue;
                }
            }
            // Empty slot — magenta so missing textures are obvious during debugging
            for (int i = 0; i < BYTES_PER_TEXTURE; i += 4)
            {
                buf.put(base + i + 0, (byte) 0xFF); // R
                buf.put(base + i + 1, (byte) 0x00); // G
                buf.put(base + i + 2, (byte) 0xFF); // B
                buf.put(base + i + 3, (byte) 0xFF); // A
            }
        }

        textureProvider.setBrightness(savedBrightness);
        VarHandle.releaseFence(); // all texture writes visible before ready flag
        buf.putInt(OFF_READY, 1);
    }
}
