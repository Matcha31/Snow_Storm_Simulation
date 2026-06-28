#include "application.hpp"
#include "opengl_object.hpp"
#include "program.hpp"
#include "utils.hpp"
#include <map>
#include <random>
#include <stdexcept>

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments) : PV227Application(initial_width, initial_height, arguments)
{
	Application::compile_shaders();
	prepare_cameras();
	prepare_textures();
	prepare_lights();
	prepare_framebuffers();
	prepare_scene();
    prepare_particles();
}

Application::~Application()
{
    // Cloud mask resources
    glDeleteFramebuffers(1, &cloud_mask_fbo);
    glDeleteTextures(1, &cloud_mask_tex);

    // Depth mask resources
    glDeleteFramebuffers(1, &depth_mask_fbo);
    glDeleteTextures(1, &sky_depth_tex);
    glDeleteTextures(1, &surface_mask_tex);

    // Particles resources
    glDeleteBuffers(1, &particle_buffer);
    glDeleteVertexArrays(1, &particle_vao);
    glDeleteFramebuffers(1, &accumulation_fbo);
    glDeleteTextures(1, &accumulation_tex);

    glDeleteFramebuffers(1, &accumulation_blur_fbo);
    glDeleteTextures(1, &accumulation_blur_tex);
}

// ----------------------------------------------------------------------------
// Shaderes
// ----------------------------------------------------------------------------
void Application::compile_shaders()
{
	default_unlit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "unlit.frag");
	default_lit_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "lit.frag");
	display_texture_program = ShaderProgram(lecture_shaders_path / "full_screen_quad.vert", lecture_shaders_path / "display_texture.frag");
    cloud_mask_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "cloud_mask.frag");
    depth_mask_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "depth_mask.frag");
    // Particle programs
    particle_program.add_vertex_shader(lecture_shaders_path /   "particle.vert");
    particle_program.add_geometry_shader(lecture_shaders_path / "particle.geom");
    particle_program.add_fragment_shader(lecture_shaders_path / "particle.frag");
    particle_program.link();
    particle_update_program.add_compute_shader(lecture_shaders_path / "particle_update.comp");
    particle_update_program.link();
    particle_accumulation_program.add_vertex_shader(lecture_shaders_path /   "particle_accumulation.vert");
    particle_accumulation_program.add_geometry_shader(lecture_shaders_path / "particle_accumulation.geom");
    particle_accumulation_program.add_fragment_shader(lecture_shaders_path / "particle_accumulation.frag");
    particle_accumulation_program.link();

    blur_program = ShaderProgram(lecture_shaders_path / "full_screen_quad.vert", lecture_shaders_path / "blur.frag");
    object_snow_program = ShaderProgram(lecture_shaders_path / "object.vert", lecture_shaders_path / "object_snow.frag");

    snow_terrain_program.add_vertex_shader(lecture_shaders_path / "snow_terrain.vert");
    snow_terrain_program.add_fragment_shader(lecture_shaders_path / "snow_terrain.frag");
    snow_terrain_program.add_tess_control_shader(lecture_shaders_path / "snow_terrain.tesc");
    snow_terrain_program.add_tess_evaluation_shader(lecture_shaders_path / "snow_terrain.tese");
    snow_terrain_program.link();

    snow_terrain_depth_program.add_vertex_shader(lecture_shaders_path / "snow_terrain.vert");
    snow_terrain_depth_program.add_fragment_shader(lecture_shaders_path / "depth_mask.frag");
    snow_terrain_depth_program.add_tess_control_shader(lecture_shaders_path / "snow_terrain.tesc");
    snow_terrain_depth_program.add_tess_evaluation_shader(lecture_shaders_path / "snow_terrain.tese");
    snow_terrain_depth_program.link();

	std::cout << "Shaders are reloaded." << std::endl;
}

// ----------------------------------------------------------------------------
// Initialize Scene
// ----------------------------------------------------------------------------
void Application::prepare_cameras()
{
	// Sets the default camera position.
	camera.set_eye_position(glm::radians(-10.f), glm::radians(30.f), 30.f);
	camera_ubo.set_projection(glm::perspective(glm::radians(45.f), static_cast<float>(this->width) / static_cast<float>(this->height), 0.1f, 5000.0f));
	camera_ubo.update_opengl_data();

    // 26x26-> 13,-13 + small margin : 15,-15
    sky_camera_ubo.set_projection(glm::ortho(-15.f, 15.f, -15.f, 15.f, 0.1f, 60.f));
    // Looking from above the scene
    sky_camera_ubo.set_view(glm::lookAt(glm::vec3(0.0f, 50.0f, 0.0f), glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
    sky_camera_ubo.update_opengl_data();
}

void Application::prepare_textures()
{
	snow_albedo_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_albedo.png");
	TextureUtils::set_texture_2d_parameters(snow_albedo_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	snow_normal_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_normal.png");
	TextureUtils::set_texture_2d_parameters(snow_normal_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	snow_height_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_height.png");
	TextureUtils::set_texture_2d_parameters(snow_height_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	snow_roughness_tex = TextureUtils::load_texture_2d(lecture_textures_path / "snow_roughness.png");
	TextureUtils::set_texture_2d_parameters(snow_roughness_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	particle_tex = TextureUtils::load_texture_2d(lecture_textures_path / "star.png");
	TextureUtils::set_texture_2d_parameters(particle_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

	circle_tex = TextureUtils::load_texture_2d(lecture_textures_path / "circle.png");
	TextureUtils::set_texture_2d_parameters(circle_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	farmhouse_tex = TextureUtils::load_texture_2d(lecture_textures_path / "farmhouse.jpg");
	TextureUtils::set_texture_2d_parameters(farmhouse_tex, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	tree_texture = TextureUtils::load_texture_2d(lecture_textures_path / "tree_cold.png");
	TextureUtils::set_texture_2d_parameters(tree_texture, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
}

void Application::prepare_lights()
{
	// The rest is set in the update scene method.
	phong_lights_ubo.set_global_ambient(glm::vec3(0.0f));
}

void Application::prepare_framebuffers()
{
    // Cloud mask framebuffer
	glCreateTextures(GL_TEXTURE_2D, 1, &cloud_mask_tex);
	glTextureStorage2D(cloud_mask_tex, 1, GL_RGBA8, cloud_mask_reso, cloud_mask_reso); // allocate mem
    // Filtering
	glTextureParameteri(cloud_mask_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // smoother edges
	glTextureParameteri(cloud_mask_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    // Wrapping
	glTextureParameteri(cloud_mask_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(cloud_mask_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glCreateFramebuffers(1, &cloud_mask_fbo);
	glNamedFramebufferTexture(cloud_mask_fbo, GL_COLOR_ATTACHMENT0, cloud_mask_tex, 0);

	GLenum draw_buffers[] = {GL_COLOR_ATTACHMENT0};
	glNamedFramebufferDrawBuffers(cloud_mask_fbo, 1, draw_buffers);
	glNamedFramebufferReadBuffer(cloud_mask_fbo, GL_COLOR_ATTACHMENT0);

	if (glCheckNamedFramebufferStatus(cloud_mask_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		throw std::runtime_error("Cloud mask framebuffer is incomplete.");
	}

    // Depth mask framebuffer
    glCreateTextures(GL_TEXTURE_2D, 1, &sky_depth_tex);
    glTextureStorage2D(sky_depth_tex, 1, GL_DEPTH_COMPONENT32F, sky_tex_reso, sky_tex_reso);
    glTextureParameteri(sky_depth_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(sky_depth_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(sky_depth_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(sky_depth_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(sky_depth_tex, GL_TEXTURE_COMPARE_MODE, GL_NONE);

    glCreateTextures(GL_TEXTURE_2D, 1, &surface_mask_tex);
    glTextureStorage2D(surface_mask_tex, 1, GL_RGBA8, sky_tex_reso, sky_tex_reso);
    glTextureParameteri(surface_mask_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(surface_mask_tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(surface_mask_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(surface_mask_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &depth_mask_fbo);
    glNamedFramebufferTexture(depth_mask_fbo, GL_DEPTH_ATTACHMENT, sky_depth_tex, 0);
    glNamedFramebufferTexture(depth_mask_fbo, GL_COLOR_ATTACHMENT0, surface_mask_tex, 0);

    glNamedFramebufferDrawBuffers(depth_mask_fbo, 1, draw_buffers);
    glNamedFramebufferReadBuffer(depth_mask_fbo, GL_COLOR_ATTACHMENT0);

    const GLfloat clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    glClearNamedFramebufferfv(accumulation_fbo, GL_COLOR, 0, clear_color);

    if (glCheckNamedFramebufferStatus(depth_mask_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Depth mask framebuffer is incomplete.");
    }

    // Accumulation
    glCreateTextures(GL_TEXTURE_2D, 1, &accumulation_tex);
    // 2 floating points channels : many can accumulate on the same area
    glTextureStorage2D(accumulation_tex, 1, GL_RG32F, sky_tex_reso, sky_tex_reso);
    glTextureParameteri(accumulation_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(accumulation_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(accumulation_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(accumulation_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &accumulation_fbo);
    glNamedFramebufferTexture(accumulation_fbo, GL_COLOR_ATTACHMENT0, accumulation_tex, 0);
    glNamedFramebufferDrawBuffers(accumulation_fbo, 1, draw_buffers);
    glNamedFramebufferReadBuffer(accumulation_fbo, GL_COLOR_ATTACHMENT0);

    if (glCheckNamedFramebufferStatus(accumulation_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Accumulation framebuffer is incomplete.");
    }

    // Blurring 
    glCreateTextures(GL_TEXTURE_2D, 1, &accumulation_blur_tex);
    glTextureStorage2D(accumulation_blur_tex, 1, GL_RG32F, sky_tex_reso, sky_tex_reso);
    glTextureParameteri(accumulation_blur_tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(accumulation_blur_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(accumulation_blur_tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(accumulation_blur_tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glCreateFramebuffers(1, &accumulation_blur_fbo);
    glNamedFramebufferTexture(accumulation_blur_fbo, GL_COLOR_ATTACHMENT0, accumulation_blur_tex, 0);
    glNamedFramebufferDrawBuffers(accumulation_blur_fbo, 1, draw_buffers);
    glNamedFramebufferReadBuffer(accumulation_blur_fbo, GL_COLOR_ATTACHMENT0);

    if (glCheckNamedFramebufferStatus(accumulation_blur_fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        throw std::runtime_error("Accumulation blur framebuffer is incomplete.");
    }

    glClearNamedFramebufferfv(accumulation_blur_fbo, GL_COLOR, 0, clear_color);
}

void Application::resize_fullscreen_textures()
{
	camera_ubo.set_projection(glm::perspective(
		glm::radians(45.f),
		static_cast<float>(width) / static_cast<float>(height),
		0.1f,
		5000.0f
	));

	camera_ubo.update_opengl_data();
}

void Application::prepare_scene()
{
	Geometry plane = Geometry::from_file(lecture_folder_path / "models/plane.obj");
	snow_terrain_object = SceneObject(plane, ModelUBO(scale(translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) * glm::mat4(1.0f), glm::vec3(26.f, 1.f, 26.f))), white_material_ubo, snow_albedo_tex);

	Geometry farmhouse = Geometry::from_file(lecture_folder_path / "models/farmhouse.obj");
	scene_objects.emplace_back(farmhouse, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 1.7f, 1.0f)) * glm::rotate(glm::mat4(1.0f), glm::radians(-118.f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(8.0f))),
		white_material_ubo, farmhouse_tex);

	glm::mat4 up = glm::rotate(glm::mat4(1.0f), glm::radians(-90.f), glm::vec3(1.0f, 0.0f, 0.0f));
	Geometry tree1 = Geometry::from_file(lecture_folder_path / "models/trees/tree_type_01.obj", false);
	Geometry tree2 = Geometry::from_file(lecture_folder_path / "models/trees/tree_type_02.obj", false);
	Geometry tree3 = Geometry::from_file(lecture_folder_path / "models/trees/tree_type_03.obj", false);
	Geometry tree4 = Geometry::from_file(lecture_folder_path / "models/trees/tree_type_04.obj", false);
	Geometry tree5 = Geometry::from_file(lecture_folder_path / "models/trees/tree_type_05.obj", false);
	scene_objects.emplace_back(tree1, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 7.5f)) * up), white_material_ubo, tree_texture);
	scene_objects.emplace_back(tree2, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(7.1f, 0.0f, 6.25f)) * up), white_material_ubo, tree_texture);
	scene_objects.emplace_back(tree3, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(-9.6f, 0.0f, 4.3f)) * up), white_material_ubo, tree_texture);
	scene_objects.emplace_back(tree4, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(7.0f, 0.0f, 0.0f)) * up), white_material_ubo, tree_texture);
	scene_objects.emplace_back(tree5, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 2.5f)) * up), white_material_ubo, tree_texture);
	scene_objects.emplace_back(tree1, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(-7.4f, 0.0f, 5.06)) * up), white_material_ubo, tree_texture);
	scene_objects.emplace_back(tree3, ModelUBO(glm::translate(glm::mat4(1.0f), glm::vec3(-8.4f, 0.0f, -8.75)) * up), white_material_ubo, tree_texture);

	// Prepares the cloud model - rest is set in the update method.
	Geometry cloud = Geometry::from_file(lecture_folder_path / "models/cloud.obj");
	cloud_object = SceneObject(cloud, ModelUBO(up), white_material_ubo);

	// The light model is placed based on the value from UI so we update its model matrix later.
	// We are also not adding it to the scene_objects since we render it separately.
	light_object1 = SceneObject(sphere, ModelUBO(), white_material_ubo);
	light_object2 = SceneObject(sphere, ModelUBO(), white_material_ubo);
}

void Application::initialize_particles(int particle_count)
{
	std::vector<Particle> particles(particle_count);

    // Deterministic distribution
	std::mt19937 generator(0);
    std::uniform_real_distribution<float> delay_distribution(0.0f, 5.0f);

    float elevation = 12.0f + cloud_size / 2.0f - 5.0f;
    glm::vec3 base_position = glm::vec3(0.0f, elevation, 0.0f);
    // glm::vec3 base_position = glm::vec3(0.0f, elevation - cloud_size * 0.75f, 0.0f);

	for (int i = 0; i < particle_count; i++)
	{
		particles[i].position = glm::vec4(base_position, 1.0f);
		particles[i].velocity_delay = glm::vec4(0.0f, 0.0f, 0.0f, delay_distribution(generator));
        // Set to unreleased : they are just candidates
		particles[i].flags = glm::ivec4(0, 0, 0, 0);
	}

	glNamedBufferData(particle_buffer, particles.size() * sizeof(Particle), particles.data(), GL_DYNAMIC_DRAW);

	current_snow_count = particle_count;
}

void Application::prepare_particles(){
    // How OpenGL should interpret the data
    glCreateVertexArrays(1, &particle_vao);
    // Stores the data
    glCreateBuffers(1, &particle_buffer);

	initialize_particles(current_snow_count);
}

// ----------------------------------------------------------------------------
// Update
// ----------------------------------------------------------------------------
void Application::update_particles(float delta)
{
	if (current_snow_count <= 0) {
		return;
	}

    // Simulation time step
    // If delta is too large -> unstable
    // So we clamp it
    float max_simulation_delta = 0.033f; // ~30 FPS
    // Slow snow down visually
    float simulation_speed = 0.35f; // Tuned myself
	float simulation_delta = std::min(delta, max_simulation_delta) * simulation_speed;

    // Update particles position
	particle_update_program.use();
	particle_update_program.uniform("particle_count", current_snow_count);
	particle_update_program.uniform("delta_time", simulation_delta);
	particle_update_program.uniform("frame_index", particle_frame_index);
	particle_update_program.uniform("cloud_world_position", cloud_world_position);
	particle_update_program.uniform("cloud_size", cloud_size);
	particle_update_program.uniform("gravity", glm::vec3(0.0f, -9.81f, 0.0f));
	particle_update_program.uniform("reset_y", 0.0f);
	particle_update_program.uniform("spawn_height_range", 1.0f); // thickness of spawn volume
	particle_update_program.uniform("initial_fall_speed", 0.8f);
	particle_update_program.uniform("max_delay", 5.0f);
    particle_update_program.uniform("kill_y", -5.0f); 
    particle_update_program.uniform("collision_bias", 0.002f); // coll slightly before
	particle_update_program.uniform("collision_offset", 0.04f); // push slightly above
	particle_update_program.uniform("restitution", 0.12f); // bounciness
	particle_update_program.uniform("stop_speed", 0.35f); 

    // Need sky camera matrix to compute if in cloud mask
	sky_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, particle_buffer);
	glBindTextureUnit(0, cloud_mask_tex);
    glBindTextureUnit(1, surface_mask_tex);
    glBindTextureUnit(2, sky_depth_tex);

    // Launch multiple work groups
	GLuint work_group_count = static_cast<GLuint>((current_snow_count + 255) / 256);
	glDispatchCompute(work_group_count, 1, 1); // launch

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Sync

	particle_frame_index++;
}

void Application::accumulate_particles()
{
	if (current_snow_count <= 0)
	{
		return;
	}

	float sky_world_size = 30.0f;
	float texel_world_size = sky_world_size / static_cast<float>(sky_tex_reso);
	float accumulation_particle_size = 2.0f * impact_radius * texel_world_size;

	glBindFramebuffer(GL_FRAMEBUFFER, accumulation_fbo);
	glViewport(0, 0, sky_tex_reso, sky_tex_reso);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_ONE, GL_ONE);

	sky_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

	particle_accumulation_program.use();
	particle_accumulation_program.uniform("accumulation_particle_size", accumulation_particle_size);
	particle_accumulation_program.uniform("frame_index", particle_frame_index);
	particle_accumulation_program.uniform("max_delay", 5.0f);
	particle_accumulation_program.uniform("accumulation_strength", 0.02f);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, particle_buffer);
	glBindTextureUnit(0, circle_tex);
	glBindVertexArray(particle_vao);

	glDrawArrays(GL_POINTS, 0, current_snow_count);

	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
}

void Application::blur_accumulation_texture()
{
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	blur_program.use();
	glBindVertexArray(empty_vao);

	glBindFramebuffer(GL_FRAMEBUFFER, accumulation_blur_fbo);
	glViewport(0, 0, sky_tex_reso, sky_tex_reso);
	glBindTextureUnit(0, accumulation_tex);
	blur_program.uniform("direction", glm::vec2(1.0f / static_cast<float>(sky_tex_reso), 0.0f)); // horizontal
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);

    // Same shadder for both passes
	glBindFramebuffer(GL_FRAMEBUFFER, accumulation_fbo);
	glViewport(0, 0, sky_tex_reso, sky_tex_reso);
	glBindTextureUnit(0, accumulation_blur_tex);
	blur_program.uniform("direction", glm::vec2(0.0f, 1.0f / static_cast<float>(sky_tex_reso))); // vertical
	glDrawArrays(GL_TRIANGLES, 0, 3);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_FRAMEBUFFER_BARRIER_BIT);
}

void Application::update(float delta)
{
	PV227Application::update(delta);
    // Time btw frames
    frame_delta = delta;

	// Updates the main camera.
	const glm::vec3 eye_position = camera.get_eye_position();
	camera_ubo.set_view(glm::lookAt(eye_position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	camera_ubo.update_opengl_data();

	float radius_x = 20.0f;
	float radius_z = 15.0f;
	float height1 = 10.0f;
	float height2 = 12.0f;

	// Computes the light position.
	glm::vec3 light_position1 = glm::vec3(radius_x * cosf(gui_light_position), height1, radius_z * sinf(gui_light_position));

	// Light 2: orbits counterclockwise
	glm::vec3 light_position2 = glm::vec3(radius_x * cosf(-gui_light_position + glm::pi<float>() / 4.0f),	 // offset by 45° for visual separation
		height2, radius_z * sinf(-gui_light_position + glm::pi<float>() / 4.0f));

	// Updates the light model visible in the scene.
	light_object1.get_model_ubo().set_matrix(glm::translate(glm::mat4(1.0f), light_position1) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)));
	light_object1.get_model_ubo().update_opengl_data();

	light_object2.get_model_ubo().set_matrix(glm::translate(glm::mat4(1.0f), light_position2) * glm::scale(glm::mat4(1.0f), glm::vec3(0.2f)));
	light_object2.get_model_ubo().update_opengl_data();

    // Update desired snow count
    if (desired_snow_count != current_snow_count)
    {
        current_snow_count = desired_snow_count;
        initialize_particles(current_snow_count);
    }

	// Updates the OpenGL buffer storing the information about the light.
	phong_lights_ubo.clear();
	phong_lights_ubo.add(PhongLightData::CreatePointLight(light_position1, glm::vec3(0.1f), glm::vec3(0.9f), glm::vec3(0.2f)));
	phong_lights_ubo.add(PhongLightData::CreatePointLight(light_position2, glm::vec3(0.1f), glm::vec3(0.9f), glm::vec3(0.2f)));
	phong_lights_ubo.update_opengl_data();
}

void Application::update_cloud_location()
{
	float elevation = 12.0f + cloud_size / 2.0f - 5.f;
	if (movable_cloud)
	{
		// TODO: CORRECTIVE ASSIGNMENT
	}
	else
	{
		cloud_world_position = glm::vec3(0.0f, elevation, 0.0f);
	}
	cloud_object.get_model_ubo().set_matrix(glm::translate(glm::mat4(1.0f), cloud_world_position) * glm::scale(glm::mat4(1.0f), glm::vec3(cloud_size)));
	cloud_object.get_model_ubo().update_opengl_data();
}

// ----------------------------------------------------------------------------
// Render
// ----------------------------------------------------------------------------
void Application::render_cloud_mask()
{
    // Every frame because size and position of the cloud can change.
	glBindFramebuffer(GL_FRAMEBUFFER, cloud_mask_fbo);
	glViewport(0, 0, cloud_mask_reso, cloud_mask_reso);

    // Black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);

	sky_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

	cloud_mask_program.use();
	cloud_object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
	cloud_object.get_geometry().bind_vao();
	cloud_object.get_geometry().draw();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::render_object_to_depth_pass(const SceneObject& object, float surface_value, bool render_as_patches) const
{
	depth_mask_program.use();
	depth_mask_program.uniform("surface_value", surface_value);

	object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
	object.get_geometry().bind_vao();

    if (render_as_patches)
    {
        glPatchParameteri(GL_PATCH_VERTICES, 3);
        if (object.get_geometry().draw_elements_count > 0)
        {
            glDrawElements(GL_PATCHES, object.get_geometry().draw_elements_count, GL_UNSIGNED_INT, nullptr);
        }
        else
        {
            glDrawArrays(GL_PATCHES, 0, object.get_geometry().draw_arrays_count);
        }
    }
    else
    {
        object.get_geometry().draw();
    }
}

void Application::render_depth_pass()
{
	glBindFramebuffer(GL_FRAMEBUFFER, depth_mask_fbo);
	glViewport(0, 0, sky_tex_reso, sky_tex_reso);

    // Black
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	sky_camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

    // Renders the terrain for 0.5
    if (use_tessellated_terrain)
        render_snow_terrain_to_depth_pass();
    else 
        render_object_to_depth_pass(snow_terrain_object, 0.5f, false);

    // Renders the objects for 1.0
	for (const SceneObject& object : scene_objects)
	{
		render_object_to_depth_pass(object, 1.0f, false);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Application::render_particles()
{
    // Get main camera
	camera_ubo.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING);

	particle_program.use();
	particle_program.uniform("particle_size", 0.12f);
	particle_program.uniform("particle_alpha", 0.65f);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, particle_buffer);
	glBindTextureUnit(0, particle_tex);
	glBindVertexArray(particle_vao);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // We test depth against object but should not write to the depth buffer
    // Otherwise particles may be occluded by the ones in front
	glDepthMask(GL_FALSE);

    // One point per particle
	glDrawArrays(GL_POINTS, 0, current_snow_count);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void Application::render_snow_terrain()
{
	snow_terrain_program.use();
	snow_terrain_program.uniform("tessellation_level", snow_tessellation_level);
	snow_terrain_program.uniform("debug_displacement", debug_snow_displacement);
	snow_terrain_program.uniform("snow_height_scale", snow_height_scale);
	snow_terrain_program.uniform("max_snow_height", max_snow_height);
	snow_terrain_program.uniform("terrain_edge_width", terrain_edge_width);
	snow_terrain_program.uniform("use_accumulation_displacement", use_accumulation_displacement);
	snow_terrain_program.uniform("height_texture_tiling", height_texture_tiling);
	snow_terrain_program.uniform("height_texture_strength", height_texture_strength);
	snow_terrain_program.uniform("terrain_world_size", terrain_world_size);
	snow_terrain_program.uniform("sky_world_size", sky_world_size);
	snow_terrain_program.uniform("use_snow_normal_map", use_snow_normal_map);
	snow_terrain_program.uniform("snow_normal_tiling", snow_normal_tiling);
	snow_terrain_program.uniform("snow_normal_strength", snow_normal_strength);

	sky_camera_ubo.bind_buffer_base(5); // Binding expected by the shader
	glBindTextureUnit(1, accumulation_tex);
	glBindTextureUnit(2, snow_height_tex);
    glBindTextureUnit(3, snow_normal_tex);

	render_object(snow_terrain_object, snow_terrain_program, true);
}

void Application::render_snow_terrain_to_depth_pass() {
	snow_terrain_depth_program.use();
	snow_terrain_depth_program.uniform("tessellation_level", snow_tessellation_level);
	snow_terrain_depth_program.uniform("debug_displacement", debug_snow_displacement);
	snow_terrain_depth_program.uniform("snow_height_scale", snow_height_scale);
	snow_terrain_depth_program.uniform("max_snow_height", max_snow_height);
	snow_terrain_depth_program.uniform("terrain_edge_width", terrain_edge_width);
	snow_terrain_depth_program.uniform("use_accumulation_displacement", use_accumulation_displacement);
	snow_terrain_depth_program.uniform("height_texture_tiling", height_texture_tiling);
	snow_terrain_depth_program.uniform("height_texture_strength", height_texture_strength);
	snow_terrain_depth_program.uniform("terrain_world_size", terrain_world_size);
	snow_terrain_depth_program.uniform("sky_world_size", sky_world_size);
	snow_terrain_depth_program.uniform("surface_value", 0.5f);

	sky_camera_ubo.bind_buffer_base(5);
	glBindTextureUnit(1, accumulation_tex);
	glBindTextureUnit(2, snow_height_tex);

	render_object(snow_terrain_object, snow_terrain_depth_program, true);
}

void Application::render()
{
	// Starts measuring the elapsed time.
	glBeginQuery(GL_TIME_ELAPSED, render_time_query);

	// Updates the cloud location and size.
	update_cloud_location();
    // Renders the cloud mask.
    render_cloud_mask();
    render_depth_pass();

	if (what_to_display == DISPLAY_CLOUD_MASK)
	{
		display_texture(cloud_mask_tex);
	}
	else if (what_to_display == DISPLAY_SNOW_MASK_TERRAIN)
	{
		display_texture(accumulation_tex, 0);
	}
	else if (what_to_display == DISPLAY_SNOW_MASK_OBJECTS)
	{
		display_texture(accumulation_tex, 1);
	}
	else if (what_to_display == DISPLAY_ACCUMULATED)
	{
		display_texture(accumulation_tex, -1);
	}
	else if (what_to_display == DISPLAY_OBJECTS)
	{
		display_texture(surface_mask_tex);
	}
	else if (what_to_display == DISPLAY_DEPTH_TEXTURE)
	{
		display_texture(sky_depth_tex);
	}
	else if (what_to_display == DISPLAY_FINAL_IMAGE)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);

		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

        // Opaque objects write deph first
		render_scene_without_cloud(camera_ubo);
		render_object(cloud_object, default_lit_program, false);
		render_object(light_object1, default_unlit_program, false);
		render_object(light_object2, default_unlit_program, false);

        // Blended on top of opaque objects
        if (show_snow){
            render_particles();
            update_particles(frame_delta);
            accumulate_particles();
            blur_accumulation_texture();
        }
	}
	glBindVertexArray(0);
	glUseProgram(0);

	// Stops measuring the elapsed time.
	glEndQuery(GL_TIME_ELAPSED);

	// Waits for OpenGL - don't forget OpenGL is asynchronous.
	glFinish();

	// Evaluates the query.
	GLuint64 render_time;
	glGetQueryObjectui64v(render_time_query, GL_QUERY_RESULT, &render_time);
	fps_gpu = 1000.f / (static_cast<float>(render_time) * 1e-6f);
}

void Application::render_scene_without_cloud(CameraUBO& camera)
{
	camera.bind_buffer_base(CameraUBO::DEFAULT_CAMERA_BINDING); 
	phong_lights_ubo.bind_buffer_base(CameraUBO::DEFAULT_LIGHTS_BINDING);

	glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
	
	for (auto& object : scene_objects)
	{
		render_object_with_snow(object);
	}
    if (use_tessellated_terrain)
        render_snow_terrain();
    else
        render_object(snow_terrain_object, default_lit_program, false);

	// Just to make sure we reset to fill mode.
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Application::render_object(const SceneObject& object, const ShaderProgram& program, bool render_as_patches) const
{
	program.use();

	// Handles the textures.
	program.uniform("has_texture", object.has_texture());
	if (object.has_texture())
	{
		glBindTextureUnit(0, object.get_texture());
	}
	else
	{
		glBindTextureUnit(0, 0);
	}

	object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
	object.get_material().bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);
	object.get_geometry().bind_vao();

	if (render_as_patches)
	{
		// We must use patches if we want to use tessellation.
		glPatchParameteri(GL_PATCH_VERTICES, 3);
		if (object.get_geometry().draw_elements_count > 0)
		{
			glDrawElements(GL_PATCHES, object.get_geometry().draw_elements_count, GL_UNSIGNED_INT, nullptr);
		}
		else
		{
			glDrawArrays(GL_PATCHES, 0, object.get_geometry().draw_arrays_count);
		}
	}
	else
	{
		// Calls the standard rendering function if we do not require patches.
		object.get_geometry().draw();
	}
}

void Application::render_object_with_snow(const SceneObject& object) const
{
	object_snow_program.use();

	object_snow_program.uniform("has_texture", object.has_texture());
	object_snow_program.uniform("object_snow_strength", 2.5f);
	object_snow_program.uniform("object_snow_max", 0.95f);
	object_snow_program.uniform("top_occlusion_bias", 0.003f);

	glBindTextureUnit(0, object.has_texture() ? object.get_texture() : 0);
	glBindTextureUnit(1, accumulation_tex);
	glBindTextureUnit(2, sky_depth_tex);

	sky_camera_ubo.bind_buffer_base(5);

	object.get_model_ubo().bind_buffer_base(ModelUBO::DEFAULT_MODEL_BINDING);
	object.get_material().bind_buffer_base(PhongMaterialUBO::DEFAULT_MATERIAL_BINDING);
	object.get_geometry().bind_vao();
	object.get_geometry().draw();
}

void Application::display_texture(GLuint texture, int channel)
{
	// Binds the main window framebuffer.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);

	// Sets the clear values and clears the framebuffer.
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	// Use the proper program
	display_texture_program.use();
    display_texture_program.uniform("texture", 0);
    display_texture_program.uniform("display_channel", channel);
	// Binds the proper texture.
	glBindTextureUnit(0, texture);

	// Renders the full screen quad to evaluate every pixel.
	// Binds an empty VAO as we do not need any state.
	glBindVertexArray(empty_vao);
	// Calls a draw command with 3 vertices that are generated in vertex shader.
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Application::clear_accumulated_snow()
{
	const GLfloat clear_color[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	glClearNamedFramebufferfv(accumulation_fbo, GL_COLOR, 0, clear_color);
    glClearNamedFramebufferfv(accumulation_blur_fbo, GL_COLOR, 0, clear_color);

	initialize_particles(current_snow_count);
}

// ----------------------------------------------------------------------------
// GUI
// ----------------------------------------------------------------------------
void Application::render_ui()
{
	const float unit = ImGui::GetFontSize();

	ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoDecoration);
	ImGui::SetWindowSize(ImVec2(20 * unit, 26 * unit));
	ImGui::SetWindowPos(ImVec2(2 * unit, 2 * unit));

	ImGui::PushItemWidth(150.f);

	std::string fps_cpu_string = "FPS (CPU): ";
	ImGui::Text(fps_cpu_string.append(std::to_string(fps_cpu)).c_str());

	std::string fps_string = "FPS (GPU): ";
	ImGui::Text(fps_string.append(std::to_string(fps_gpu)).c_str());

	ImGui::SliderAngle("Light Position", &gui_light_position, 0);

	if (ImGui::Button("Clear Snow"))
	{
		clear_accumulated_snow();
	}

	ImGui::Checkbox("Show Snow", &show_snow);

	ImGui::Checkbox("Movable Cloud", &movable_cloud);

	ImGui::SliderFloat("Cloud Size", &cloud_size, 1.f, 30.f);

	ImGui::Checkbox("Wireframe", &wireframe);

    ImGui::Checkbox("Tessellated Terrain", &use_tessellated_terrain);
    ImGui::SliderFloat("Tess Level", &snow_tessellation_level, 1.0f, 64.0f);
    ImGui::SliderFloat("Debug Snow Height", &debug_snow_displacement, 0.0f, 1.0f);
    ImGui::Checkbox("Accumulation Height", &use_accumulation_displacement);
    ImGui::SliderFloat("Snow Height Scale", &snow_height_scale, 0.0f, 5.0f);
    ImGui::SliderFloat("Max Snow Height", &max_snow_height, 0.0f, 5.0f);
    ImGui::SliderFloat("Terrain Edge Width", &terrain_edge_width, 0.0f, 0.05f);
    ImGui::SliderFloat("Height Tex Tiling", &height_texture_tiling, 1.0f, 32.0f);
    ImGui::SliderFloat("Height Tex Strength", &height_texture_strength, 0.0f, 1.0f);
    ImGui::Checkbox("Snow Normal Map", &use_snow_normal_map);
    ImGui::SliderFloat("Normal Tiling", &snow_normal_tiling, 1.0f, 64.0f);
    ImGui::SliderFloat("Normal Strength", &snow_normal_strength, 0.0f, 2.0f);

	const char* particle_labels[10] = {"256", "512", "1024", "2048", "4096", "8192", "16384", "32768", "65536", "131072"};
	int exponent = static_cast<int>(log2(current_snow_count) - 8);	  // -8 because we start at 256 = 2^8
	if (ImGui::Combo("Particle Count", &exponent, particle_labels, IM_ARRAYSIZE(particle_labels)))
	{
		desired_snow_count = static_cast<int>(glm::pow(2, exponent + 8));	 // +8 because we start at 256 = 2^8
	}

	ImGui::Combo("Display", &what_to_display, DISPLAY_LABELS, IM_ARRAYSIZE(DISPLAY_LABELS));

	ImGui::SliderFloat("Impact Radius", &impact_radius, 0.5f, 15.0f);

	ImGui::End();
}

// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------
void Application::on_resize(int width, int height)
{
	PV227Application::on_resize(width, height);
	resize_fullscreen_textures();
}

void Application::on_mouse_move(double x, double y)
{
	PV227Application::on_mouse_move(x, y);
	mouse_position = glm::vec2(static_cast<float>(x), static_cast<float>(height - y));
}
