import numpy as np
from PIL import Image

def diffusion_profile(r):
    """Skin diffusion profile (6 gaussians - Jimenez 2015)"""
    g1 = np.exp(-r*r / (2*0.0064**2))
    g2 = np.exp(-r*r / (2*0.0484**2))
    g3 = np.exp(-r*r / (2*0.187**2))
    g4 = np.exp(-r*r / (2*0.567**2))
    g5 = np.exp(-r*r / (2*1.99**2))
    g6 = np.exp(-r*r / (2*7.41**2))
    return 0.233*g1 + 0.100*g2 + 0.118*g3 + 0.113*g4 + 0.358*g5 + 0.078*g6

def generate_sss_lut(size=256):
    """Generate pre-integrated skin LUT: [NdotL][curvature] → diffuse"""
    lut = np.zeros((size, size, 3), dtype=np.uint8)
    
    for y in range(size):  # NdotL (0 to 1)
        for x in range(size):  # curvature (0 to 1)
            NdotL = y / (size - 1)
            curvature = x / (size - 1)
            
            # Integrate diffusion over sphere
            # For each angle θ, sample NdotL rotated by θ, weighted by diffusion at that angle
            samples = 64
            total = 0.0
            weight_sum = 0.0
            
            for i in range(samples):
                theta = (i / samples) * np.pi  # 0 to π
                # Rotated NdotL (approximation)
                rotated_NdotL = NdotL * np.cos(theta) + np.sin(np.arccos(np.clip(NdotL, -1, 1))) * np.sin(theta)
                rotated_NdotL = max(0, min(1, rotated_NdotL))
                
                # Distance on sphere (approximation for curvature)
                arc_dist = theta * curvature
                w = diffusion_profile(arc_dist)
                
                total += rotated_NdotL * w
                weight_sum += w
            
            diffuse = total / weight_sum if weight_sum > 0 else NdotL
            
            # Apply:
            val = int(diffuse * 255)
            lut[y, x] = [val, val, val]
    
    return lut

if __name__ == "__main__":
    print("Generating SSS LUT (256x256)...")
    lut = generate_sss_lut(256)
    Image.fromarray(lut).save("sss_lut.png")
    print("✅ Saved: luts/sss_lut.png")