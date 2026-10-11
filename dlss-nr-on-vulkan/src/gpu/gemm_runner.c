/*
 * gemm_runner — run gemm_coopmat.spv on the Xe2 iGPU and write the result out.
 *
 *   gemm_runner <spv> <M> <N> <K> <A.f16> <B.f16> <C.f32>
 *
 * <spv> is a file, or — with the CMake build, which compiles every module in — a bare
 * name such as gemm_coopmat.spv for the embedded one.
 *
 * A is MxK float16, B is KxN float16, C is MxN float32, all row-major raw dumps.
 * Deliberately simple: host-visible coherent memory, one dispatch, fence wait.
 * This exists to be checked against the numpy reference, not to be fast.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>
#include "nr_shaders_embedded.h"

#ifdef _WIN32
#include <windows.h>
#endif

#define VKOK(x) do { VkResult _r = (x); if (_r != VK_SUCCESS) { \
    fprintf(stderr, "%s:%d: %s -> %d\n", __FILE__, __LINE__, #x, _r); exit(1); } } while (0)

static void *slurp(const char *path, size_t *len)
{
#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    FILE *f = NULL;
    fopen_s(&f, path, "rb");
#else
	FILE *f = fopen(path, "rb");
#endif

	if (!f) { perror(path); exit(1); }
	fseek(f, 0, SEEK_END); *len = ftell(f); fseek(f, 0, SEEK_SET);
	void *p = malloc(*len);
	if (!p) { perror("malloc"); exit(1); }
	if (fread(p, 1, *len, f) != *len) { fprintf(stderr, "short read %s\n", path); exit(1); }
	fclose(f);
	return p;
}

static uint32_t pick_memory(VkPhysicalDevice pd, uint32_t bits, VkMemoryPropertyFlags want)
{
	VkPhysicalDeviceMemoryProperties mp;
	vkGetPhysicalDeviceMemoryProperties(pd, &mp);
	for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
		if ((bits & (1u << i)) && (mp.memoryTypes[i].propertyFlags & want) == want)
			return i;
	fprintf(stderr, "no suitable memory type\n"); exit(1);
}

struct buf { VkBuffer b; VkDeviceMemory m; void *p; VkDeviceSize size; };

static struct buf make_buf(VkDevice dev, VkPhysicalDevice pd, VkDeviceSize size)
{
	struct buf o = { .size = size };
	VkBufferCreateInfo bi = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
				  .size = size,
				  .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				  .sharingMode = VK_SHARING_MODE_EXCLUSIVE };
	VKOK(vkCreateBuffer(dev, &bi, NULL, &o.b));
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(dev, o.b, &mr);
	VkMemoryAllocateInfo ai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				    .allocationSize = mr.size,
				    .memoryTypeIndex = pick_memory(pd, mr.memoryTypeBits,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) };
	VKOK(vkAllocateMemory(dev, &ai, NULL, &o.m));
	VKOK(vkBindBufferMemory(dev, o.b, o.m, 0));
	VKOK(vkMapMemory(dev, o.m, 0, size, 0, &o.p));
	return o;
}

int main(int argc, char **argv)
{
	if (argc != 8) {
		fprintf(stderr, "usage: %s <spv> <M> <N> <K> <A.f16> <B.f16> <C.f32>\n", argv[0]);
		return 2;
	}
	uint32_t M = atoi(argv[2]), N = atoi(argv[3]), K = atoi(argv[4]);

	VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				  .pApplicationName = "gemm_runner",
				  .apiVersion = VK_API_VERSION_1_3 };
	/* Behind the loader on macOS, MoltenVK is a portability driver and hidden until
	 * the instance asks for it; elsewhere the extension is absent and nothing is asked. */
	uint32_t nie = 0;
	vkEnumerateInstanceExtensionProperties(NULL, &nie, NULL);
	VkExtensionProperties *ie = calloc(nie ? nie : 1, sizeof *ie);
	vkEnumerateInstanceExtensionProperties(NULL, &nie, ie);
	int portability = 0;
	for (uint32_t i = 0; i < nie; i++)
		if (!strcmp(ie[i].extensionName, "VK_KHR_portability_enumeration")) portability = 1;
	free(ie);
	const char *iext[] = { "VK_KHR_portability_enumeration" };
	VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, .pApplicationInfo = &app,
				     .flags = portability ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0u,
				     .enabledExtensionCount = portability ? 1u : 0u,
				     .ppEnabledExtensionNames = iext };
	VkInstance inst;
	VKOK(vkCreateInstance(&ici, NULL, &inst));

	uint32_t nd = 0;
	vkEnumeratePhysicalDevices(inst, &nd, NULL);
	VkPhysicalDevice *pds = calloc(nd, sizeof *pds);
	vkEnumeratePhysicalDevices(inst, &nd, pds);
	VkPhysicalDevice pd = pds[0];
	VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(pd, &props);
	fprintf(stderr, "device: %s\n", props.deviceName);
	/* Ask for the matrix extension only where it exists; the shader handed in has to
	 * match (`gemm_portable_desc.spv` where it does not), which is the caller's job. */
	uint32_t nde = 0;
	vkEnumerateDeviceExtensionProperties(pd, NULL, &nde, NULL);
	VkExtensionProperties *de = calloc(nde ? nde : 1, sizeof *de);
	vkEnumerateDeviceExtensionProperties(pd, NULL, &nde, de);
	int coopmat = 0, subset = 0;
	for (uint32_t i = 0; i < nde; i++) {
		if (!strcmp(de[i].extensionName, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME)) coopmat = 1;
		if (!strcmp(de[i].extensionName, "VK_KHR_portability_subset")) subset = 1;
	}
	free(de);
	fprintf(stderr, "path: %s\n", coopmat ? "cooperative matrix" : "portable multiply-add (no VK_KHR_cooperative_matrix)");

	uint32_t nq = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, NULL);
	VkQueueFamilyProperties *qf = calloc(nq, sizeof *qf);
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf);
	uint32_t qi = UINT32_MAX;
	for (uint32_t i = 0; i < nq; i++)
		if (qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) { qi = i; break; }
	if (qi == UINT32_MAX) { fprintf(stderr, "no compute queue\n"); return 1; }

	/* Feature chain: cooperative matrix needs the memory model, fp16 arithmetic
	 * and 16-bit storage buffers all switched on explicitly. The integer twin
	 * (`gemm_coopmat_int8.comp`, configuration 4) needs the 8-bit pair as well; asking
	 * for both costs nothing and lets one runner check either kernel. */
	VkPhysicalDeviceCooperativeMatrixFeaturesKHR cm = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR,
		.cooperativeMatrix = VK_TRUE };
	/* The 8-bit pair is asked for only where the device has it (MoltenVK and the phones
	 * do not all): without it the float kernels are unaffected and the integer one fails
	 * to load, which is the caller's business. */
	VkPhysicalDeviceVulkan12Features have12 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
	VkPhysicalDeviceFeatures2 have2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &have12 };
	vkGetPhysicalDeviceFeatures2(pd, &have2);
	VkPhysicalDeviceVulkan12Features v12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = coopmat ? (void *)&cm : NULL, .vulkanMemoryModel = VK_TRUE,
		.vulkanMemoryModelDeviceScope = VK_TRUE, .shaderFloat16 = VK_TRUE,
		.shaderInt8 = have12.shaderInt8, .storageBuffer8BitAccess = have12.storageBuffer8BitAccess };
	VkPhysicalDeviceVulkan11Features v11 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		.pNext = &v12, .storageBuffer16BitAccess = VK_TRUE };
	VkPhysicalDeviceFeatures2 f2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, .pNext = &v11 };

	float prio = 1.0f;
	VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = qi, .queueCount = 1, .pQueuePriorities = &prio };
	const char *devext[2]; uint32_t next = 0;
	if (coopmat) devext[next++] = VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME;
	if (subset) devext[next++] = "VK_KHR_portability_subset";
	VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, .pNext = &f2,
				   .queueCreateInfoCount = 1, .pQueueCreateInfos = &qci,
				   .enabledExtensionCount = next, .ppEnabledExtensionNames = devext };
	VkDevice dev;
	VKOK(vkCreateDevice(pd, &dci, NULL, &dev));
	VkQueue queue;
	vkGetDeviceQueue(dev, qi, 0, &queue);

	/* The operand width comes from the files, not from a flag: two bytes an element is
	 * the float kernel, one byte the integer one, and a file that is neither size is a
	 * caller error worth naming rather than a buffer overrun worth debugging. The
	 * accumulator is four bytes either way — float for config 1, int32 for config 4. */
	size_t la, lb;
	void *pa = slurp(argv[5], &la), *pb = slurp(argv[6], &lb);
	size_t wanted_a = (size_t)M * K, wanted_b = (size_t)K * N;
	size_t width = la == wanted_a ? 1 : (la == wanted_a * 2 ? 2 : 0);
	if (!width || lb != wanted_b * width) {
		fprintf(stderr, "input size mismatch: A %zu, B %zu; expected %zu and %zu "
			"(int8) or %zu and %zu (fp16)\n", la, lb, wanted_a, wanted_b,
			wanted_a * 2, wanted_b * 2);
		return 1;
	}
	struct buf A = make_buf(dev, pd, (VkDeviceSize)la);
	struct buf B = make_buf(dev, pd, (VkDeviceSize)lb);
	struct buf C = make_buf(dev, pd, (VkDeviceSize)M * N * 4);
	memcpy(A.p, pa, la); memcpy(B.p, pb, lb); memset(C.p, 0, C.size);

	VkDescriptorSetLayoutBinding bind[3];
	for (int i = 0; i < 3; i++)
		bind[i] = (VkDescriptorSetLayoutBinding){ .binding = i, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
							  .descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };
	VkDescriptorSetLayoutCreateInfo dsl = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
						.bindingCount = 3, .pBindings = bind };
	VkDescriptorSetLayout setlayout;
	VKOK(vkCreateDescriptorSetLayout(dev, &dsl, NULL, &setlayout));

	VkPushConstantRange pcr = { .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT, .offset = 0, .size = 12 };
	VkPipelineLayoutCreateInfo pli = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					   .setLayoutCount = 1, .pSetLayouts = &setlayout,
					   .pushConstantRangeCount = 1, .pPushConstantRanges = &pcr };
	VkPipelineLayout playout;
	VKOK(vkCreatePipelineLayout(dev, &pli, NULL, &playout));

	/* As libxmx resolves a shader: a bare name (`gemm_coopmat.spv`) is the module compiled
	 * into this executable; a path is a file; a path whose file is missing falls back to
	 * the embedded module of the same base name. */
	size_t spvlen = 0;
	const void *spv = NULL;
	const struct nr_embedded_shader *embedded = NULL;
	const char *base = argv[1];
	for (const char *q = argv[1]; *q; q++)
		if (*q == '/' || *q == '\\') base = q + 1;
#ifdef NR_EMBEDDED_SHADERS
	for (size_t i = 0; i < nr_embedded_shader_count; i++)
		if (!strcmp(nr_embedded_shaders[i].name, base)) embedded = &nr_embedded_shaders[i];
#endif
	if (embedded && base == argv[1]) { spv = embedded->data; spvlen = embedded->size; }
	else if (embedded) {
		FILE *probe = fopen(argv[1], "rb");
		if (probe) fclose(probe); else { spv = embedded->data; spvlen = embedded->size; }
	}
	if (!spv) spv = slurp(argv[1], &spvlen);
	VkShaderModuleCreateInfo smi = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					 .codeSize = spvlen, .pCode = spv };
	VkShaderModule sm;
	VKOK(vkCreateShaderModule(dev, &smi, NULL, &sm));

	VkComputePipelineCreateInfo cpi = { .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			   .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = sm, .pName = "main" },
		.layout = playout };
	VkPipeline pipe;
	VKOK(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &cpi, NULL, &pipe));

	VkDescriptorPoolSize ps = { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 3 };
	VkDescriptorPoolCreateInfo dpi = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					   .maxSets = 1, .poolSizeCount = 1, .pPoolSizes = &ps };
	VkDescriptorPool pool;
	VKOK(vkCreateDescriptorPool(dev, &dpi, NULL, &pool));
	VkDescriptorSetAllocateInfo dsa = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					    .descriptorPool = pool, .descriptorSetCount = 1, .pSetLayouts = &setlayout };
	VkDescriptorSet set;
	VKOK(vkAllocateDescriptorSets(dev, &dsa, &set));

	VkDescriptorBufferInfo dbi[3] = { { A.b, 0, A.size }, { B.b, 0, B.size }, { C.b, 0, C.size } };
	VkWriteDescriptorSet w[3];
	for (int i = 0; i < 3; i++)
		w[i] = (VkWriteDescriptorSet){ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .dstSet = set,
					       .dstBinding = i, .descriptorCount = 1,
					       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .pBufferInfo = &dbi[i] };
	vkUpdateDescriptorSets(dev, 3, w, 0, NULL);

	VkCommandPoolCreateInfo cpci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, .queueFamilyIndex = qi };
	VkCommandPool cpool;
	VKOK(vkCreateCommandPool(dev, &cpci, NULL, &cpool));
	VkCommandBufferAllocateInfo cba = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					    .commandPool = cpool, .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, .commandBufferCount = 1 };
	VkCommandBuffer cb;
	VKOK(vkAllocateCommandBuffers(dev, &cba, &cb));
	VkCommandBufferBeginInfo cbb = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
					 .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
	VKOK(vkBeginCommandBuffer(cb, &cbb));
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_COMPUTE, pipe);
	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_COMPUTE, playout, 0, 1, &set, 0, NULL);
	uint32_t pcv[3] = { M, N, K };
	vkCmdPushConstants(cb, playout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 12, pcv);
	vkCmdDispatch(cb, N / 16, M / 8, 1);
	VKOK(vkEndCommandBuffer(cb));

	VkFenceCreateInfo fi = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VkFence fence;
	VKOK(vkCreateFence(dev, &fi, NULL, &fence));
	VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO, .commandBufferCount = 1, .pCommandBuffers = &cb };
	VKOK(vkQueueSubmit(queue, 1, &si, fence));
	VKOK(vkWaitForFences(dev, 1, &fence, VK_TRUE, 30ull * 1000 * 1000 * 1000));

#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    FILE *out = NULL;
    fopen_s(&out, argv[7], "wb");
#else
	FILE *out = fopen(argv[7], "wb");
#endif

	if (!out) { perror(argv[7]); return 1; }
	fwrite(C.p, 1, C.size, out);
	fclose(out);
	fprintf(stderr, "wrote %llu bytes to %s\n", (unsigned long long)C.size, argv[7]);

	vkDestroyFence(dev, fence, NULL);
	vkDestroyCommandPool(dev, cpool, NULL);
	vkDestroyDescriptorPool(dev, pool, NULL);
	vkDestroyPipeline(dev, pipe, NULL);
	vkDestroyShaderModule(dev, sm, NULL);
	vkDestroyPipelineLayout(dev, playout, NULL);
	vkDestroyDescriptorSetLayout(dev, setlayout, NULL);
	vkDestroyDevice(dev, NULL);
	vkDestroyInstance(inst, NULL);
	return 0;
}
