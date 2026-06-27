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

- Create sky camera
- Render cloud into off screen texture
- Store cloud projection as B&W texture
- Use it as valib snow spawn area

## 1.2 Top-down depth

- Render scene from sky camera into top-down depth texture
- Exclude cloud and particles
- Differentiate terrain and objects in mask texture

## 3. Snow Particules

### 3.1 Initialization & Rendering

- Store particles in SSBO
- Draw one point per particle and expand it into a billboard quad
- Make the face the camera
- Sample snowflakes texture into frag shader
- Render it with blending and depth test

### 3.2 GPU Update & Release

- Update particles on GPU
- Keep unreleased until delay expires
- Randomize candidate spawn positions inside cloud mask
- Project into sky space and release inside cloud mask
- Apply gravity

### 3.3 Depth Collision & Hit Classification

- Project particle into sky space
- Sample top-down depth texture
- Compare particle depth with scene depth
- If particle is below the stored surface depth -> collision
- Bounce using reconstructed surface normal
- If velocity becomes low -> mark as hit terrain/object

## 4. Particule Accumulation

### 4.1 Accumulation Texture

- Keep stopped particles as hit terrain or object
- Render them from sky camera
- Write impact into accumulation texture
- Store terrain in R, object in G
- Blend with previous frame
- Reset accumated particles
- Clear accumulation texture when clicking clear snow

### 4.2 Blur Accumulation Texture

- 

### 4.3 Whitening Effect on Objects


## 5. Tesselation Snow Cover

-- 
### AI Use

- Find vec4 for particles attributes to have aligned GPU memory
- Create rndom number generator in GLSL
