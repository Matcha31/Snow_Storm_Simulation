// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
//
// Copyright (c) Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#pragma once
#include "camera_ubo.hpp"
#include "light_ubo.hpp"
#include "program.hpp"
#include "pv227_application.hpp"
#include "scene_object.hpp"
#include <glm/ext/vector_int4.hpp>

struct Particle {
    // vec4 safer for GPU allignment (AI used to find this out)
    // everything is 16-byte aligned : 3 * 16 = 48
    glm::vec4 position; // xyz = wordspace, w = for projection
    glm::vec4 velocity_delay; // xyz = velocity , w = delay
    glm::ivec4 flags; // x = hit terrain, y = hit object, z = released, w = padding
};

class Application : public PV227Application
{
	// ----------------------------------------------------------------------------
	// Variables (Geometry)
	// ----------------------------------------------------------------------------
protected:
	/** The snow terrain object. */
	SceneObject snow_terrain_object;
	/** The tree objects. */
	std::vector<SceneObject> scene_objects;

	/** The mouse position that can be used to update cloud position. */
	glm::vec2 mouse_position;
	/** The cloud world position. */
	glm::vec3 cloud_world_position;

	/** The scene object storing information about the cloud model. */
	SceneObject cloud_object;
	/** The scene object representing the first light. */
	SceneObject light_object1;
	/** The scene object representing the second light. */
	SceneObject light_object2;

	// ----------------------------------------------------------------------------
	// Variables (Materials)
	// ----------------------------------------------------------------------------
protected:

	// ----------------------------------------------------------------------------
	// Variables (Textures)
	// ----------------------------------------------------------------------------
protected:
	/** The texture for cloud. */
	GLuint circle_tex;
	/** The texture for the snow particles. */
	GLuint particle_tex;
	/** The snow albedo texture. */
	GLuint snow_albedo_tex;
	/** The snow normal texture. */
	GLuint snow_normal_tex;
	/** The snow height texture. */
	GLuint snow_height_tex;
	/** The snow roughness */
	GLuint snow_roughness_tex;
	/** The farmhouse texture */
	GLuint farmhouse_tex;
	/** The tree texture */
	GLuint tree_texture;

    /** The cloud mask texture */
    GLuint cloud_mask_tex;
    /** The depth mask texture */
    GLuint sky_depth_tex;
    GLuint surface_mask_tex;
    GLuint accumulation_tex;
    GLuint accumulation_blur_tex;

	// ----------------------------------------------------------------------------
	// Variables (Light)
	// ----------------------------------------------------------------------------
protected:
	/** The UBO storing the data about lights - positions, colors, etc. */
	PhongLightsUBO phong_lights_ubo;

	// ----------------------------------------------------------------------------
	// Variables (Camera)
	// ----------------------------------------------------------------------------
protected:
	/** The UBO storing the information about the camera. */
	CameraUBO camera_ubo;
    CameraUBO sky_camera_ubo;

	// ----------------------------------------------------------------------------
	// Variables (Shaders)
	// ----------------------------------------------------------------------------
protected:
	/** The program for rendering textures. */
	ShaderProgram display_texture_program;

    ShaderProgram cloud_mask_program;
    ShaderProgram depth_mask_program;
    ShaderProgram particle_program;
    ShaderProgram particle_update_program;
    ShaderProgram particle_accumulation_program;
    ShaderProgram blur_program;

	// ----------------------------------------------------------------------------
	// Variables (Frame Buffers)
	// ----------------------------------------------------------------------------
    GLuint cloud_mask_fbo;
    GLuint depth_mask_fbo;
    GLuint accumulation_fbo;
    GLuint accumulation_blur_fbo;

    // ----------------------------------------------------------------------------
    // Variables (Buffers)
    // ----------------------------------------------------------------------------
    GLuint particle_buffer; // SSBO storing all particles
    GLuint particle_vao; // empty VAO for particle rendering
protected:
    // ----------------------------------------------------------------------------
    // Variables (Others)
    // ----------------------------------------------------------------------------
    int cloud_mask_reso = 1024;
    int sky_tex_reso = 1024;
    float particle_size = 0.12f;
    float frame_delta = 0.0f;
    int particle_frame_index = 0;

	// ----------------------------------------------------------------------------
	// Variables (GUI)
	// ----------------------------------------------------------------------------
protected:
	/** The light position set in the GUI. */
	float gui_light_position = glm::radians(314.f);

	/** The maximum number of snow particles. */
	int max_snow_count = 131072;

	/** The desired snow particle count. */
	int desired_snow_count = 16384;

	/** The current snow particle count. */
	int current_snow_count = 256;

	/** The flag determining if a snow should be visible. */
	bool show_snow = true;

	/** The constants identifying what can be displayed on the screen. */
	const int DISPLAY_CLOUD_MASK = 0;
	const int DISPLAY_SNOW_MASK_TERRAIN = 1;
	const int DISPLAY_SNOW_MASK_OBJECTS = 2;
	const int DISPLAY_ACCUMULATED = 3;
	const int DISPLAY_OBJECTS = 4;
	const int DISPLAY_DEPTH_TEXTURE = 5;
	const int DISPLAY_FINAL_IMAGE = 6;
	/** The GUI labels for the constants above. */
	const char* DISPLAY_LABELS[7] = {"Cloud Mask", "Snow Mask Terrain", "Snow Mask Objects", "Accumulated", "Objects", "Depth", "Final Image"};

	/** The flag determining what will be displayed on the screen right now. */
	int what_to_display = DISPLAY_FINAL_IMAGE;

	/** The flag determining polygon fill mode. */
	bool wireframe = false;

	/** Shoulbe the cloud be movable. */
	bool movable_cloud = false;
	
	/** Cloud size. */
	float cloud_size = 8.f;

	/** The impact radius of the snow. */
	float impact_radius = 5.f;

	// ----------------------------------------------------------------------------
	// Constructors
	// ----------------------------------------------------------------------------
public:
	Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

	/** Destroys the {@link Application} and releases the allocated resources. */
	~Application() override;

	// ----------------------------------------------------------------------------
	// Shaders
	// ----------------------------------------------------------------------------
	/**
	 * {@copydoc PV227Application::compile_shaders}
	 */
	void compile_shaders() override;

	// ----------------------------------------------------------------------------
	// Initialize Scene
	// ----------------------------------------------------------------------------
public:
	/** Prepares the required cameras. */
	void prepare_cameras();

	/** Prepares the required textures. */
	void prepare_textures();

	/** Prepares the lights. */
	void prepare_lights();

	/** Prepares the scene objects. */
	void prepare_scene();

	/** Prepares the frame buffer objects. */
	void prepare_framebuffers();

	/** Resizes the full screen textures match the window. */
	void resize_fullscreen_textures();

    void initialize_particles();

    void prepare_particles();
    void initialize_particles(int count);

	// ----------------------------------------------------------------------------
	// Update
	// ----------------------------------------------------------------------------
	/**
	 * {@copydoc PV227Application::update}
	 */
	void update(float delta) override;

	/**
	 * Updates the cloud location.
	 */
	void update_cloud_location();

    void update_particles(float delta);

    void accumulate_particles();
    void blur_accumulation_texture();

	// ----------------------------------------------------------------------------
	// Render
	// ----------------------------------------------------------------------------
public:
	/** @copydoc PV227Application::render */
	void render() override;

	/** Renders the whole scene without cloud. */
	void render_scene_without_cloud(CameraUBO& camera, bool depth_pass);

	/** Renders the specified texture over the whole screen. */
	void display_texture(GLuint texture, int channel = 0);

	/** Renders the specified object. */
	void render_object(const SceneObject& object, const ShaderProgram& program, bool render_as_patches) const;

	/** Clears the accumulated snow. */
	void clear_accumulated_snow();

    /** Render cloud mask */
    void render_cloud_mask();

    /** Render depth mask */
    void render_depth_pass();
    void render_object_to_depth_pass(const SceneObject& object, float surface_value, bool render_as_patches) const;

    /** Render particles */
    void render_particles();

	// ----------------------------------------------------------------------------
	// GUI
	// ----------------------------------------------------------------------------
public:
	/** @copydoc PV227Application::render_ui */
	void render_ui() override;

	// ----------------------------------------------------------------------------
	// Input Events
	// ----------------------------------------------------------------------------
public:
	/** @copydoc PV227Application::on_resize */
	void on_resize(int width, int height) override;

	/** @copydoc PV227Application::on_mouse_move */
	void on_mouse_move(double x, double y) override;
};
