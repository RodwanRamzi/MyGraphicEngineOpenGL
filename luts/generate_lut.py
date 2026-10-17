import numpy as np
from PIL import Image

def generate_identity_lut(size=32):
    """Generate identity LUT — no color change."""
    lut = np.zeros((size * size, size, 3), dtype=np.uint8)
    
    for b in range(size):
        for g in range(size):
            for r in range(size):
                idx = b * size + g
                lut[idx, r] = [
                    int(r * 255 / (size - 1)),
                    int(g * 255 / (size - 1)),
                    int(b * 255 / (size - 1))
                ]
    
    return lut


def generate_mec_lut(size=32):
    """Mirror's Edge Catalyst style — cool shadows, warm highlights."""
    lut = np.zeros((size * size, size, 3), dtype=np.float32)
    
    for b in range(size):
        for g in range(size):
            for r in range(size):
                idx = b * size + g
                # Normalize to [0, 1]
                nr = r / (size - 1)
                ng = g / (size - 1)
                nb = b / (size - 1)
                
                # MEC grade: boost reds, crush blues in highlights
                nr_out = nr ** 0.9          # Slight lift
                ng_out = ng ** 1.0          # Neutral
                nb_out = nb ** 1.1          # Slight crush
                
                # Boost red channel on highlights
                nr_out = min(1.0, nr_out + 0.05 * nr_out)
                # Cool shadows — add blue in darks
                shadow_mask = 1.0 - (nr + ng + nb) / 3.0
                nb_out = min(1.0, nb_out + 0.08 * shadow_mask)
                
                lut[idx, r] = [nr_out, ng_out, nb_out]
    
    return (lut * 255).astype(np.uint8)


def save_lut(lut, filename):
    """Save LUT as horizontal strip PNG."""
    size = lut.shape[1]
    height = size
    width = size * size
    
    # Rearrange to (height, width, 3)
    img = np.zeros((height, width, 3), dtype=np.uint8)
    
    for b in range(size):
        for g in range(size):
            for r in range(size):
                x = b * size + r
                y = g
                img[y, x] = lut[b * size + g, r]
    
    Image.fromarray(img).save(filename)
    print(f"✅ Saved: {filename}")


if __name__ == "__main__":
    print("Generating LUTs...")
    
    identity = generate_identity_lut(32)
    save_lut(identity, "lut_identity.png")
    
    mec = generate_mec_lut(32)
    save_lut(mec, "lut_mec.png")
    
    print("Done.")