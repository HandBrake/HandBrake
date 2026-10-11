/* Present real frames through the layer, on a surface that needs no screen.
 *
 * Everything else about the layer is tested without ever presenting: the wire protocol
 * against a stand-in daemon, the interface detector on arrays, the manifest against a
 * loader. The one thing left out was the part that runs inside `vkQueuePresentKHR` — the
 * copy out, the copy back and, since `NR_LAYER_SYNC=semaphore`, who waits for whom. That
 * needed a game, and a game is not a test.
 *
 * `VK_EXT_headless_surface` gives a swapchain with no window. The frames are 64x32 and the
 * daemon is a Python stand-in that checks what it is sent and answers with a colour of its
 * own, so both directions are observable:
 *
 *   - the first pass over the images is cleared to a colour that says which frame it is,
 *     and the daemon must receive exactly that;
 *   - the second pass records nothing at all, so each image still holds what the layer
 *     wrote into it last time round. The daemon must now receive its own answer back.
 *
 * ## Why this is not the obvious loop (phase68, phase69)
 *
 * The obvious loop — one command buffer, one semaphore pair, `vkQueueWaitIdle` every
 * frame — cannot tell a correct layer from one that never waits on the game's semaphores
 * at all. Both pass, and so does synchronization validation, because nothing ever
 * overlaps and there is no hazard to find. Three things fix that, and all three are
 * needed:
 *
 * **Frames in flight.** FRAMES_IN_FLIGHT slots, each with its own command buffer, fence
 * and acquire semaphore, and no queue idle anywhere. Work from one frame is still
 * running when the next is recorded.
 *
 * **A render-finished semaphore per image, not per frame.** A present waits on one, and
 * there is no way to learn when a present has finished with it — no fence, no callback.
 * Reusing it on the next frame is the defect `VUID-vkAcquireNextImageKHR-semaphore-01779`
 * and `VUID-vkQueueSubmit-pSignalSemaphores-00067` report, and it is what the first
 * attempt at this hit. Keyed on the image index it is safe: that semaphore is untouched
 * until the same image comes round again, and the fence that last wrote the image is
 * waited on before it does.
 *
 * **Work that is genuinely unfinished when the capture starts.** The draw clears the
 * image to the *wrong* colour, runs a serial chain of NR_TEST_STALL clears on a large
 * scratch image, and only then clears it to the right one. A copy that does not wait
 * reads the wrong colour, which the stand-in daemon sees and reports.
 *
 * And the copy has to be able to race the draw at all, which on one in-order queue it
 * cannot: submissions on a single queue are not reordered, so a missing wait is invisible
 * however long the stall. The test therefore **presents from a different queue family**
 * when the device has one that can present and can hold a copy — the arrangement
 * `nr_layer.c` singles out as dangerous, and the one VKD3D-Proton produces. The swapchain
 * is then CONCURRENT across both families so no ownership transfer is owed.
 *
 * `NR_TEST_PRESENT_QUEUE=same` forces the single-queue arrangement back, which is how the
 * claim above was measured rather than assumed.
 *
 * Exit 0 and a line per frame; `test_present.py` drives it in both sync modes.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define WIDTH 64
#define HEIGHT 32
#define FRAMES_IN_FLIGHT 2
#define SCRATCH 1024                  /* the stall's working set, square */
#define STALL_DEFAULT 2048           /* serial clears of it per drawn frame, ~20 ms */
#define MAX_IMAGES 8

#define CHECK(what, r) do { \
	VkResult code_ = (r); \
	if (code_ != VK_SUCCESS) { fprintf(stderr, "%s: %d\n", what, (int)code_); return 2; } \
} while (0)

static unsigned env_count(const char *name, unsigned fallback)
{
	const char *text = getenv(name);
	if (!text || !*text) return fallback;
	long value = strtol(text, NULL, 10);
	return value < 0 ? 0 : (unsigned)value;
}

/* A barrier that orders one transfer write against the next, on a whole image. */
static void serialise(VkCommandBuffer commands, VkImage image, VkImageLayout layout)
{
	VkImageMemoryBarrier b = { .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				   .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				   .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				   .oldLayout = layout, .newLayout = layout,
				   .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				   .image = image,
				   .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };
	vkCmdPipelineBarrier(commands, VK_PIPELINE_STAGE_TRANSFER_BIT,
			     VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &b);
}

int main(int argc, char **argv)
{
	unsigned rounds = argc > 1 ? (unsigned)atoi(argv[1]) : 2;
	unsigned stall = env_count("NR_TEST_STALL", STALL_DEFAULT);
	const char *layers[] = { "VK_LAYER_dlssnr_intel" };
	/* On macOS the loader hides MoltenVK — a "portability" driver — until the instance
	 * asks for it, which a game does too; the layer sits behind whatever the game asked. */
	const char *instance_ext[] = { VK_KHR_SURFACE_EXTENSION_NAME,
				       VK_EXT_HEADLESS_SURFACE_EXTENSION_NAME,
				       VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME };
#ifdef __APPLE__
	const uint32_t instance_ext_count = 3;
	const VkInstanceCreateFlags instance_flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#else
	const uint32_t instance_ext_count = 2;
	const VkInstanceCreateFlags instance_flags = 0;
#endif
	VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				  .pApplicationName = "nr present test",
				  .apiVersion = VK_API_VERSION_1_3 };
	VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				     .pApplicationInfo = &app, .flags = instance_flags,
				     /* `NR_TEST_NO_LAYER=1` runs the same frames with no layer
				      * at all, which is how a failure is told apart from a
				      * failure of ours */
				     .enabledLayerCount = getenv("NR_TEST_NO_LAYER") ? 0 : 1,
				     .ppEnabledLayerNames = layers,
				     .enabledExtensionCount = instance_ext_count,
				     .ppEnabledExtensionNames = instance_ext };
	VkInstance instance;
	CHECK("vkCreateInstance", vkCreateInstance(&ici, NULL, &instance));

	uint32_t n = 0;
	vkEnumeratePhysicalDevices(instance, &n, NULL);
	if (!n) { fprintf(stderr, "no physical device\n"); return 2; }
	VkPhysicalDevice *pds = calloc(n, sizeof *pds);
	vkEnumeratePhysicalDevices(instance, &n, pds);
	VkPhysicalDevice pd = pds[0];
	free(pds);

	PFN_vkCreateHeadlessSurfaceEXT create_surface = (PFN_vkCreateHeadlessSurfaceEXT)
		vkGetInstanceProcAddr(instance, "vkCreateHeadlessSurfaceEXT");
	if (!create_surface) { fprintf(stderr, "no headless surface\n"); return 2; }
	VkHeadlessSurfaceCreateInfoEXT hs = {
		.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT };
	VkSurfaceKHR surface;
	CHECK("vkCreateHeadlessSurfaceEXT", create_surface(instance, &hs, NULL, &surface));

	uint32_t families = 0, draw_family = UINT32_MAX, present_family = UINT32_MAX;
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &families, NULL);
	VkQueueFamilyProperties *props = calloc(families, sizeof *props);
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &families, props);
	for (uint32_t i = 0; i < families; i++) {
		VkBool32 can_present = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &can_present);
		if (can_present && (props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
			draw_family = i;
			break;
		}
	}
	if (draw_family == UINT32_MAX) {
		fprintf(stderr, "no present queue\n");
		free(props);
		return 2;
	}
	/* A present queue of a different family, if there is one that can also hold the
	 * layer's copy. Transfer-only will not do: `family_can_capture` in the layer refuses
	 * it, correctly, and then there is no capture to race. */
	const char *want = getenv("NR_TEST_PRESENT_QUEUE");
	if (!(want && !strcmp(want, "same")))
		for (uint32_t i = 0; i < families; i++) {
			VkBool32 can_present = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(pd, i, surface, &can_present);
			if (i != draw_family && can_present && props[i].queueCount &&
			    (props[i].queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT))) {
				present_family = i;
				break;
			}
		}
	free(props);
	int split = present_family != UINT32_MAX;
	if (!split) present_family = draw_family;

	float priority = 1.0f;
	VkDeviceQueueCreateInfo qci[2] = {
		{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		  .queueFamilyIndex = draw_family, .queueCount = 1, .pQueuePriorities = &priority },
		{ .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		  .queueFamilyIndex = present_family, .queueCount = 1, .pQueuePriorities = &priority } };
	const char *device_ext[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				   .queueCreateInfoCount = split ? 2u : 1u,
				   .pQueueCreateInfos = qci,
				   .enabledExtensionCount = 1,
				   .ppEnabledExtensionNames = device_ext };
	VkDevice device;
	CHECK("vkCreateDevice", vkCreateDevice(pd, &dci, NULL, &device));
	VkQueue draw_queue, present_queue;
	vkGetDeviceQueue(device, draw_family, 0, &draw_queue);
	vkGetDeviceQueue(device, present_family, 0, &present_queue);

	/* A deliberate, harmless invalid call, so a caller can prove the validation layer is
	 * actually in the chain rather than assume it from a silent run. Nothing else here
	 * runs: this mode exists only to produce one known message. */
	if (getenv("NR_TEST_VALIDATION_PROBE")) {
		VkBufferCreateInfo bad = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
					   .size = 0,          /* VUID-VkBufferCreateInfo-size-00912 */
					   .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					   .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
		VkBuffer probe = VK_NULL_HANDLE;
		if (vkCreateBuffer(device, &bad, NULL, &probe) == VK_SUCCESS && probe)
			vkDestroyBuffer(device, probe, NULL);
		printf("validation probe issued\n");
		fflush(stdout);
		vkDestroyDevice(device, NULL);
		vkDestroySurfaceKHR(instance, surface, NULL);
		vkDestroyInstance(instance, NULL);
		return 0;
	}

	VkSurfaceCapabilitiesKHR caps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surface, &caps);
	uint32_t images_wanted = caps.minImageCount < 2 ? 2 : caps.minImageCount;
	uint32_t shared[2] = { draw_family, present_family };
	/* Only colour attachment, deliberately: the layer patches TRANSFER_SRC/DST into the
	 * create info on the way down, exactly as it does for a game that never asked.
	 * `NR_TEST_TRANSFER_USAGE` asks for them here instead, which is what the validation
	 * layer needs — it sits above this one and never sees the patch (phase68). */
	VkSwapchainCreateInfoKHR sci = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR, .surface = surface,
		.minImageCount = images_wanted, .imageFormat = VK_FORMAT_B8G8R8A8_UNORM,
		.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		.imageExtent = { WIDTH, HEIGHT }, .imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			    | (getenv("NR_TEST_TRANSFER_USAGE")
			       ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
			       : 0),
		.imageSharingMode = split ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = split ? 2u : 0u,
		.pQueueFamilyIndices = split ? shared : NULL,
		.preTransform = caps.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = VK_PRESENT_MODE_FIFO_KHR, .clipped = VK_TRUE };
	VkSwapchainKHR chain;
	CHECK("vkCreateSwapchainKHR", vkCreateSwapchainKHR(device, &sci, NULL, &chain));
	uint32_t image_count = 0;
	vkGetSwapchainImagesKHR(device, chain, &image_count, NULL);
	if (image_count > MAX_IMAGES) image_count = MAX_IMAGES;
	VkImage images[MAX_IMAGES];
	vkGetSwapchainImagesKHR(device, chain, &image_count, images);

	printf("present queue: %s (draw family %u, present family %u); stall %u; "
	       "%u images; %u frames in flight\n",
	       split ? "separate" : "same", draw_family, present_family, stall,
	       image_count, FRAMES_IN_FLIGHT);
	fflush(stdout);

	VkCommandPoolCreateInfo cpi = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
					.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
					.queueFamilyIndex = draw_family };
	VkCommandPool pool;
	CHECK("vkCreateCommandPool", vkCreateCommandPool(device, &cpi, NULL, &pool));
	VkCommandBuffer commands[FRAMES_IN_FLIGHT];
	VkCommandBufferAllocateInfo cba = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					    .commandPool = pool,
					    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					    .commandBufferCount = FRAMES_IN_FLIGHT };
	CHECK("vkAllocateCommandBuffers", vkAllocateCommandBuffers(device, &cba, commands));

	VkSemaphoreCreateInfo semi = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
				  .flags = VK_FENCE_CREATE_SIGNALED_BIT };
	VkSemaphore acquired[FRAMES_IN_FLIGHT];
	VkFence slot_fence[FRAMES_IN_FLIGHT];
	for (unsigned i = 0; i < FRAMES_IN_FLIGHT; i++) {
		CHECK("vkCreateSemaphore", vkCreateSemaphore(device, &semi, NULL, &acquired[i]));
		CHECK("vkCreateFence", vkCreateFence(device, &fci, NULL, &slot_fence[i]));
	}
	/* Per image, not per frame: a present waits on this one and never says when it is
	 * done with it. See the header. */
	VkSemaphore rendered[MAX_IMAGES];
	VkFence image_fence[MAX_IMAGES];
	for (uint32_t i = 0; i < image_count; i++) {
		CHECK("vkCreateSemaphore", vkCreateSemaphore(device, &semi, NULL, &rendered[i]));
		image_fence[i] = VK_NULL_HANDLE;
	}

	/* The stall's scratch. Never presented, never copied by the layer: it exists only to
	 * keep the draw queue busy after the wrong colour has been written. */
	VkImage scratch = VK_NULL_HANDLE;
	VkDeviceMemory scratch_memory = VK_NULL_HANDLE;
	if (stall) {
		VkImageCreateInfo ii = { .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
					 .imageType = VK_IMAGE_TYPE_2D,
					 .format = VK_FORMAT_R8G8B8A8_UNORM,
					 .extent = { SCRATCH, SCRATCH, 1 },
					 .mipLevels = 1, .arrayLayers = 1,
					 .samples = VK_SAMPLE_COUNT_1_BIT,
					 .tiling = VK_IMAGE_TILING_OPTIMAL,
					 .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
					 .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
					 .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED };
		CHECK("vkCreateImage", vkCreateImage(device, &ii, NULL, &scratch));
		VkMemoryRequirements need;
		vkGetImageMemoryRequirements(device, scratch, &need);
		VkPhysicalDeviceMemoryProperties mp;
		vkGetPhysicalDeviceMemoryProperties(pd, &mp);
		uint32_t type = UINT32_MAX;
		for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
			if ((need.memoryTypeBits & (1u << i)) &&
			    (mp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
				type = i;
				break;
			}
		if (type == UINT32_MAX) { fprintf(stderr, "no device-local memory\n"); return 2; }
		VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
					     .allocationSize = need.size, .memoryTypeIndex = type };
		CHECK("vkAllocateMemory", vkAllocateMemory(device, &mai, NULL, &scratch_memory));
		CHECK("vkBindImageMemory", vkBindImageMemory(device, scratch, scratch_memory, 0));
	}
	int scratch_ready = 0;

	unsigned char initialised[MAX_IMAGES] = { 0 };
	unsigned ready = 0, untouched = 0;
	unsigned untouched_wanted = rounds > 1 ? (rounds - 1) * image_count : 0;
	unsigned guard = image_count * 32 + untouched_wanted + 32;
	int status = 0;
	for (unsigned frame = 0; frame < guard; frame++) {
		if (ready >= image_count && untouched >= untouched_wanted) break;
		unsigned slot = frame % FRAMES_IN_FLIGHT;
		CHECK("vkWaitForFences",
		      vkWaitForFences(device, 1, &slot_fence[slot], VK_TRUE, 10ull * 1000000000ull));
		uint32_t index = 0;
		CHECK("vkAcquireNextImageKHR",
		      vkAcquireNextImageKHR(device, chain, UINT64_MAX, acquired[slot],
					    VK_NULL_HANDLE, &index));
		if (index >= image_count) { fprintf(stderr, "image index out of range\n"); return 2; }
		/* Whoever wrote this image last may still be running; its `rendered` semaphore
		 * and its contents are only free once that fence is signalled. */
		if (image_fence[index] != VK_NULL_HANDLE)
			CHECK("vkWaitForFences",
			      vkWaitForFences(device, 1, &image_fence[index], VK_TRUE,
					      10ull * 1000000000ull));
		image_fence[index] = slot_fence[slot];
		CHECK("vkResetFences", vkResetFences(device, 1, &slot_fence[slot]));

		int draw = ready < image_count;
		if (draw) {
			if (!initialised[index]) { initialised[index] = 1; ready++; }
		} else {
			untouched++;
		}

		VkCommandBufferBeginInfo bi = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
		vkResetCommandBuffer(commands[slot], 0);
		CHECK("vkBeginCommandBuffer", vkBeginCommandBuffer(commands[slot], &bi));
		if (draw) {
			VkImageSubresourceRange all = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			VkImageMemoryBarrier into = {
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = images[index], .subresourceRange = all };
			/* TRANSFER, not TOP_OF_PIPE: the submit below waits on `acquired` with
			 * pWaitDstStageMask = TRANSFER, and a layout transition placed before
			 * that stage is not covered by the wait — a WRITE_AFTER_READ against
			 * vkAcquireNextImageKHR, which synchronization validation reports. */
			vkCmdPipelineBarrier(commands[slot], VK_PIPELINE_STAGE_TRANSFER_BIT,
					     VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL,
					     1, &into);
			if (stall) {
				if (!scratch_ready) {
					VkImageMemoryBarrier s = into;
					s.image = scratch;
					vkCmdPipelineBarrier(commands[slot],
							     VK_PIPELINE_STAGE_TRANSFER_BIT,
							     VK_PIPELINE_STAGE_TRANSFER_BIT,
							     0, 0, NULL, 0, NULL, 1, &s);
					scratch_ready = 1;
				}
				/* The wrong answer first, so a copy that does not wait has
				 * something wrong to find. */
				VkClearColorValue wrong = { .float32 = { 1.0f, 0.0f, 0.0f, 1.0f } };
				vkCmdClearColorImage(commands[slot], images[index],
						     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						     &wrong, 1, &all);
				serialise(commands[slot], images[index],
					  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				for (unsigned i = 0; i < stall; i++) {
					VkClearColorValue grind = {
						.float32 = { (float)(i & 7) / 8.0f, 0.5f, 0.5f, 1.0f } };
					vkCmdClearColorImage(commands[slot], scratch,
							     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							     &grind, 1, &all);
					serialise(commands[slot], scratch,
						  VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
				}
			}
			/* The frame number in the blue channel, so the daemon can say which
			 * frame it was handed and a stale copy is visible rather than merely
			 * suspected. */
			VkClearColorValue colour = { .float32 = { 0.25f, 0.5f,
								  (float)(frame + 1) / 255.0f, 1.0f } };
			vkCmdClearColorImage(commands[slot], images[index],
					     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &colour, 1, &all);
			VkImageMemoryBarrier out = into;
			out.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			out.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			out.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			out.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			vkCmdPipelineBarrier(commands[slot], VK_PIPELINE_STAGE_TRANSFER_BIT,
					     VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL,
					     0, NULL, 1, &out);
		}
		CHECK("vkEndCommandBuffer", vkEndCommandBuffer(commands[slot]));

		/* Always submitted, drawn or not: the present waits on `rendered[index]`, and
		 * the slot's fence is what makes reuse of both safe. An untouched frame is an
		 * empty command buffer, which keeps that bookkeeping in one shape. */
		VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
				    .waitSemaphoreCount = 1, .pWaitSemaphores = &acquired[slot],
				    .pWaitDstStageMask = &stage,
				    .commandBufferCount = 1, .pCommandBuffers = &commands[slot],
				    .signalSemaphoreCount = 1, .pSignalSemaphores = &rendered[index] };
		CHECK("vkQueueSubmit", vkQueueSubmit(draw_queue, 1, &si, slot_fence[slot]));

		VkPresentInfoKHR pi = { .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
					.waitSemaphoreCount = 1,
					.pWaitSemaphores = &rendered[index],
					.swapchainCount = 1, .pSwapchains = &chain,
					.pImageIndices = &index };
		VkResult presented = vkQueuePresentKHR(present_queue, &pi);
		if (presented != VK_SUCCESS && presented != VK_SUBOPTIMAL_KHR)
			CHECK("vkQueuePresentKHR", presented);
		printf("frame %u image %u state=%s\n", frame, index, draw ? "drawn" : "untouched");
		fflush(stdout);
	}

	vkDeviceWaitIdle(device);
	if (scratch) vkDestroyImage(device, scratch, NULL);
	if (scratch_memory) vkFreeMemory(device, scratch_memory, NULL);
	for (uint32_t i = 0; i < image_count; i++)
		vkDestroySemaphore(device, rendered[i], NULL);
	for (unsigned i = 0; i < FRAMES_IN_FLIGHT; i++) {
		vkDestroySemaphore(device, acquired[i], NULL);
		vkDestroyFence(device, slot_fence[i], NULL);
	}
	vkDestroyCommandPool(device, pool, NULL);
	vkDestroySwapchainKHR(device, chain, NULL);
	vkDestroyDevice(device, NULL);
	vkDestroySurfaceKHR(instance, surface, NULL);
	vkDestroyInstance(instance, NULL);
	return status;
}
