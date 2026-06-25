# Render / update pipeline considered

Per frame:

1. Update camera, lights, cloud position
2. Resize/reinitialize resources if needed
3. Render cloud mask from sky camera
4. Render top-down depth + object/terrain mask from sky camera
5. Render final opaque scene:
      - objects with whitening shader
      - snow terrain with tessellation/displacement shader
      - cloud and light spheres
6. Render snow particles in main camera view:
      - depth test enabled
      - depth write disabled
      - alpha blending enabled
7. Compute particle update on GPU:
      - release delayed particles inside cloud mask
      - apply gravity
      - collide against top-down depth texture
      - mark stopped particles as terrain-hit or object-hit
8. Accumulate stopped particles into accumulation texture
9. Blur accumulation texture
10. Display selected debug texture if GUI asks for it

# Outline

## 1. Cloud mask

Find where the paticules may spawn.

- Add sky camera
- Create cloud mask texture
- Create frame buffer
- Render cloud mask every frame

## 2. Top-down depth

What is the first thing hit by a falling snow particule (terrain or object).

- Create depth / mask shader to classify objects and terrain
- Create depth buffer
- Render terrain into depth buffer
- Render one object into depth buffer
- Make texture in grayscale in display (all red otherwise)
