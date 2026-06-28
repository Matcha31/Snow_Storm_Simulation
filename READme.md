# Render / update pipeline

### Per frame:

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

---

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

Store persistent snow information.

- Keep stopped particles as hit terrain or object
- Render them from sky camera
- Write impact into accumulation texture
- Store terrain in R channel and object in G channel
- Blend with previous frame (additive blending)
- Reset accumated particles to reuse
- Clear accumulation texture when clicking clear snow

### 4.2 Blur Accumulation Texture

Smooth stored snow.

- Use separable blur (for perf)
- 1st pass: horizontal blur from accumulation texture into temp texture
- 2nd pass: vertical blur from temp texture into accumulation texture
- Avoid writing and reading same texture at the same time
- Clear blur textures when clicking clear snow

### 4.3 Whitening Effect on Objects

Use object accumulation to whiten objects.

- Project object into sky space
- Use accumulation in G channel to whiten object
- Use top-down texture to avoid whitening hidden fragments (under)
- Clear whitening texture when clicking clear snow

## 5. Tessellation Snow Cover

### 5.1 Tesselation Terrain Mesh

- Render snow terrain with GL_PATCHES
- Set tessellation level (tess control shader)
- Interpolate generated vertices (tess evaluation shader)
- Use gl_TessCoord as barycentric coordinates
- Debug with wireframe and constant displacement

### 5.2  Displacement from Accumulation Texture

Use accumulation to create 3D snow cover.

- Project terrain into sky space
- Use accumulation in R channel to convert into world space height
- Clamp to max height
- Displace vertex upward (Y axis)
- Keep edges fixed with border test

### 5.3 Texture variations

Make snow cover more realistic.

- Sample snow_height_tex with terrain UVs
- Use height texture as local variation on accumulated snow height
- Keep accumulation texture in sky UVs and material texture in terrain UVs

### 5.4 Snow Lighting and Normal Mapping

Make lighting match the displaced snow.

- Reconstruct snow normal from neighboring height samples
- Estimate slope with left/right/top/bottom height differences
- Use reconstructed normal in Phong lighting
- Build tangent and bitangent from height field
- Sample snow_normal_tex with terrain UVs
- Convert normal map from [0, 1] to [-1, 1]
- Transform sampled normal using tangent, bitangent and macro normal

## 6. Tunning

- Resize fullscreen texture (camera UBO)
- Tune parameters using UI

---

### AI Use

- Design SSBO particle layout with vec4/ivec4 alignment
- Create hash based random number generator in GLSL
- Find how to blur depending on radius to eliminate rain like artifacts (blur radius and sigma)
