/*
 * slm_occupancy — how many workgroups a subslice keeps resident, read off the run time
 * of the same arithmetic under different amounts of declared shared memory.
 *
 * On Intel with Mesa's ANV, a pipeline's shared memory reaches the hardware twice: as
 * what each workgroup is given, rounded up to an allocation size of at least 1 KB, and
 * as the subslice's SLM partition, which intel_compute_preferred_slm_calc_encode_size()
 * sizes from the *unrounded* bytes. A 32-invocation workgroup declaring 256 B is given
 * 1 KB inside a 16 KB partition: 16 resident where the threads allow 64. So this
 * program runs about four times slower at 256 B than at 1 KB, twice as slow at 512 B,
 * and 1.3x slower at 1.25-1.5 KB than at 2 KB. INTEL_DEBUG=bat shows the two fields.
 * notes/improve-shared-memory.md.
 *
 * Build and run:
 *   glslangValidator --target-env vulkan1.3 -o work/slm_occupancy.spv src/probe/slm_occupancy.comp
 *   cc -O2 -Iwork/vulkan-headers/include -o work/slm_occupancy src/probe/slm_occupancy.c -lvulkan
 *   work/slm_occupancy work/slm_occupancy.spv
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#define CHECK(call) do {                                                        \
	VkResult r_ = (call);                                                   \
	if (r_ != VK_SUCCESS) {                                                 \
		fprintf(stderr, "%s:%d: %s: VkResult %d\n", __FILE__, __LINE__, \
			#call, r_);                                             \
		exit(1);                                                        \
	}                                                                       \
} while (0)

enum { GROUPS = 65536, WARMUP = 2, RUNS = 7 };

static const uint32_t sizes[] = {
	64, 128, 256, 384, 512, 768, 1024, 1280, 1536, 1792, 2048, 3072, 4096,
};
#define NSIZES (sizeof sizes / sizeof *sizes)

static uint32_t *load_spirv(const char *path, size_t *bytes)
{
	FILE *f = fopen(path, "rb");
	if (!f) {
		perror(path);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	long n = ftell(f);
	fseek(f, 0, SEEK_SET);
	uint32_t *code = malloc(n);
	if (n <= 0 || n % 4 || fread(code, 1, n, f) != (size_t)n) {
		fprintf(stderr, "%s: not a SPIR-V module\n", path);
		exit(1);
	}
	fclose(f);
	*bytes = n;
	return code;
}

static int by_value(const void *a, const void *b)
{
	double x = *(const double *)a, y = *(const double *)b;
	return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s slm_occupancy.spv\n", argv[0]);
		return 2;
	}
	size_t code_bytes;
	uint32_t *code = load_spirv(argv[1], &code_bytes);

	VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				  .pApplicationName = "slm_occupancy",
				  .apiVersion = VK_API_VERSION_1_3 };
	VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				     .pApplicationInfo = &app };
	VkInstance inst;
	CHECK(vkCreateInstance(&ici, NULL, &inst));

	uint32_t n = 1;
	VkPhysicalDevice pd;
	VkResult r = vkEnumeratePhysicalDevices(inst, &n, &pd);
	if ((r != VK_SUCCESS && r != VK_INCOMPLETE) || n == 0) {
		fprintf(stderr, "no Vulkan device\n");
		return 1;
	}

	VkPhysicalDeviceDriverProperties driver = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES };
	VkPhysicalDeviceSubgroupSizeControlProperties subgroup = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_PROPERTIES,
		.pNext = &driver };
	VkPhysicalDeviceProperties2 props = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &subgroup };
	vkGetPhysicalDeviceProperties2(pd, &props);

	VkPhysicalDeviceVulkan13Features has13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	VkPhysicalDeviceFeatures2 has = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
					  .pNext = &has13 };
	vkGetPhysicalDeviceFeatures2(pd, &has);
	/* Pin the subgroup size where the device allows it, so that one workgroup is one
	 * hardware thread and the resident count is a count of threads. */
	int pin = has13.subgroupSizeControl && subgroup.minSubgroupSize <= 32
		  && subgroup.maxSubgroupSize >= 32
		  && (subgroup.requiredSubgroupSizeStages & VK_SHADER_STAGE_COMPUTE_BIT);

	uint32_t nq = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, NULL);
	VkQueueFamilyProperties *qf = calloc(nq, sizeof *qf);
	vkGetPhysicalDeviceQueueFamilyProperties(pd, &nq, qf);
	uint32_t family = UINT32_MAX;
	for (uint32_t i = 0; i < nq && family == UINT32_MAX; i++)
		if ((qf[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && qf[i].timestampValidBits)
			family = i;
	free(qf);
	if (family == UINT32_MAX) {
		fprintf(stderr, "no compute queue with timestamps\n");
		return 1;
	}

	float priority = 1.0f;
	VkDeviceQueueCreateInfo qci = { .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = family, .queueCount = 1,
					.pQueuePriorities = &priority };
	VkPhysicalDeviceVulkan13Features want13 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.subgroupSizeControl = pin ? VK_TRUE : VK_FALSE };
	VkDeviceCreateInfo dci = { .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				   .pNext = &want13, .queueCreateInfoCount = 1,
				   .pQueueCreateInfos = &qci };
	VkDevice dev;
	CHECK(vkCreateDevice(pd, &dci, NULL, &dev));
	VkQueue queue;
	vkGetDeviceQueue(dev, family, 0, &queue);

	/* The sink the shader never writes, but must be able to. */
	VkBufferCreateInfo bci = { .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .size = 16,
				   .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT };
	VkBuffer sink;
	CHECK(vkCreateBuffer(dev, &bci, NULL, &sink));
	VkMemoryRequirements mr;
	vkGetBufferMemoryRequirements(dev, sink, &mr);
	uint32_t type = 0;
	while (!(mr.memoryTypeBits & (1u << type)))
		type++;
	VkMemoryAllocateInfo mai = { .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
				     .allocationSize = mr.size, .memoryTypeIndex = type };
	VkDeviceMemory memory;
	CHECK(vkAllocateMemory(dev, &mai, NULL, &memory));
	CHECK(vkBindBufferMemory(dev, sink, memory, 0));

	VkDescriptorSetLayoutBinding binding = {
		.binding = 0, .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
		.descriptorCount = 1, .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };
	VkDescriptorSetLayoutCreateInfo dslci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 1, .pBindings = &binding };
	VkDescriptorSetLayout dsl;
	CHECK(vkCreateDescriptorSetLayout(dev, &dslci, NULL, &dsl));
	VkDescriptorPoolSize pool_size = { .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
					   .descriptorCount = 1 };
	VkDescriptorPoolCreateInfo dpci = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
					    .maxSets = 1, .poolSizeCount = 1,
					    .pPoolSizes = &pool_size };
	VkDescriptorPool dpool;
	CHECK(vkCreateDescriptorPool(dev, &dpci, NULL, &dpool));
	VkDescriptorSetAllocateInfo dsai = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
					     .descriptorPool = dpool, .descriptorSetCount = 1,
					     .pSetLayouts = &dsl };
	VkDescriptorSet set;
	CHECK(vkAllocateDescriptorSets(dev, &dsai, &set));
	VkDescriptorBufferInfo dbi = { .buffer = sink, .range = VK_WHOLE_SIZE };
	VkWriteDescriptorSet write = { .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				       .dstSet = set, .descriptorCount = 1,
				       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				       .pBufferInfo = &dbi };
	vkUpdateDescriptorSets(dev, 1, &write, 0, NULL);

	VkPipelineLayoutCreateInfo plci = { .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
					    .setLayoutCount = 1, .pSetLayouts = &dsl };
	VkPipelineLayout layout;
	CHECK(vkCreatePipelineLayout(dev, &plci, NULL, &layout));
	VkShaderModuleCreateInfo smci = { .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
					  .codeSize = code_bytes, .pCode = code };
	VkShaderModule module;
	CHECK(vkCreateShaderModule(dev, &smci, NULL, &module));

	VkQueryPoolCreateInfo qpci = { .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
				       .queryType = VK_QUERY_TYPE_TIMESTAMP, .queryCount = 2 };
	VkQueryPool queries;
	CHECK(vkCreateQueryPool(dev, &qpci, NULL, &queries));
	VkCommandPoolCreateInfo cpci = { .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
					 .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
					 .queueFamilyIndex = family };
	VkCommandPool cpool;
	CHECK(vkCreateCommandPool(dev, &cpci, NULL, &cpool));
	VkCommandBufferAllocateInfo cbai = { .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
					     .commandPool = cpool,
					     .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
					     .commandBufferCount = 1 };
	VkCommandBuffer cmd;
	CHECK(vkAllocateCommandBuffers(dev, &cbai, &cmd));
	VkFenceCreateInfo fci = { .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	VkFence fence;
	CHECK(vkCreateFence(dev, &fci, NULL, &fence));

	printf("%s — %s %s\n", props.properties.deviceName, driver.driverName, driver.driverInfo);
	printf("%d workgroups of 32 invocations, subgroup size %s\n\n", GROUPS,
	       pin ? "32 (required)" : "left to the compiler");

	double ms[NSIZES];
	for (size_t s = 0; s < NSIZES; s++) {
		uint32_t words = sizes[s] / 4;
		VkSpecializationMapEntry entry = { .constantID = 0, .size = sizeof words };
		VkSpecializationInfo spec = { .mapEntryCount = 1, .pMapEntries = &entry,
					      .dataSize = sizeof words, .pData = &words };
		VkPipelineShaderStageRequiredSubgroupSizeCreateInfo required = {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO,
			.requiredSubgroupSize = 32 };
		VkComputePipelineCreateInfo cpci2 = {
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			.stage = { .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				   .pNext = pin ? &required : NULL,
				   .stage = VK_SHADER_STAGE_COMPUTE_BIT, .module = module,
				   .pName = "main", .pSpecializationInfo = &spec },
			.layout = layout };
		VkPipeline pipeline;
		CHECK(vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &cpci2, NULL, &pipeline));

		double runs[RUNS];
		for (int i = 0; i < WARMUP + RUNS; i++) {
			VkCommandBufferBeginInfo cbbi = {
				.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
				.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT };
			CHECK(vkBeginCommandBuffer(cmd, &cbbi));
			vkCmdResetQueryPool(cmd, queries, 0, 2);
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1,
						&set, 0, NULL);
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queries, 0);
			vkCmdDispatch(cmd, GROUPS, 1, 1);
			vkCmdWriteTimestamp(cmd, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queries, 1);
			CHECK(vkEndCommandBuffer(cmd));
			VkSubmitInfo si = { .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
					    .commandBufferCount = 1, .pCommandBuffers = &cmd };
			CHECK(vkQueueSubmit(queue, 1, &si, fence));
			CHECK(vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX));
			CHECK(vkResetFences(dev, 1, &fence));
			uint64_t stamp[2];
			CHECK(vkGetQueryPoolResults(dev, queries, 0, 2, sizeof stamp, stamp,
						    sizeof *stamp,
						    VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
			if (i >= WARMUP)
				runs[i - WARMUP] = (double)(stamp[1] - stamp[0])
						   * props.properties.limits.timestampPeriod * 1e-6;
		}
		qsort(runs, RUNS, sizeof *runs, by_value);
		ms[s] = runs[RUNS / 2];
		vkDestroyPipeline(dev, pipeline, NULL);
	}

	double at_1k = 0;
	for (size_t s = 0; s < NSIZES; s++)
		if (sizes[s] == 1024)
			at_1k = ms[s];
	printf("  shared bytes        ms   against 1 KB\n");
	for (size_t s = 0; s < NSIZES; s++)
		printf("  %12u  %8.2f   %6.2fx\n", sizes[s], ms[s], ms[s] / at_1k);

	vkDestroyFence(dev, fence, NULL);
	vkDestroyCommandPool(dev, cpool, NULL);
	vkDestroyQueryPool(dev, queries, NULL);
	vkDestroyShaderModule(dev, module, NULL);
	vkDestroyPipelineLayout(dev, layout, NULL);
	vkDestroyDescriptorPool(dev, dpool, NULL);
	vkDestroyDescriptorSetLayout(dev, dsl, NULL);
	vkDestroyBuffer(dev, sink, NULL);
	vkFreeMemory(dev, memory, NULL);
	vkDestroyDevice(dev, NULL);
	vkDestroyInstance(inst, NULL);
	free(code);
	return 0;
}
