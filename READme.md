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

## 1. Masks

### 1.1 Cloud mask

Find where the paticules may spawn.

- Add sky camera
- Create cloud mask texture
- Create frame buffer
- Render cloud mask every frame

## 1.2 Top-down depth

What is the first thing hit by a falling snow particule (terrain or object).

- Create depth / mask shader to classify objects and terrain
- Create depth buffer
- Render terrain into depth buffer
- Render one object into depth buffer
- Make texture in grayscale in display (all red otherwise)

## 3. Snow Particules

### 3.1 Particule Buffer & Static Billboard

- Create particles on the CPU
- Send them to the GPU in an SSBO
- Render each particle as a camera-facing quad
- Use the snowflake/star texture
- Respect depth with the scene
- Show Snow checkbox
- Particle Count combo

### 3.2 GPU Update & Release

- Start unreleased and invisible
- Wait for their delay
- Try random positions around the cloud
- Sample the cloud mask
- Release only if they are inside the projected cloud and delay is over
- Fall with gravity once released
- Reset when they reach the ground for now

### 3.3 Depth Collision & Hit Classification

- Project particle into sky space
- Sample top-down depth texture
- Compare particle depth with scene depth
- If particle is below the stored surface depth -> collision
- Bounce using reconstructed surface normal
- If velocity becomes low -> mark as hit terrain/object

-- 
### AI Use

- Find vec4 for particles attributes to have aligned GPU memory
- Create rndom number generator in GLSL
