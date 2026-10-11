/*
 * coopmat_probe — enumerate VkCooperativeMatrixPropertiesKHR on this machine.
 *
 * Resolves notes/CLAUDE.md open questions 1 and 3:
 *   1. the actual cooperative matrix configuration table (vulkaninfo does not print it)
 *   3. whether FP32 accumulation is available for the BF16 configs
 *
 * Build: cc -O2 -Iwork/vulkan-headers/include -o work/coopmat_probe \
 *              src/probe/coopmat_probe.c -lvulkan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * The probe prints what three newer extensions add to the component-type table, but
 * it must build against whatever vulkan_core.h a distribution ships (Ubuntu 24.04 has
 * 1.3.275). Where the header predates an extension, its enum values, struct and
 * extension name are given here as the registry defines them; a header that has
 * them defines the VK_<ext> macro and its definitions are used instead. The values are
 * the registry's, so the device's answer is read correctly either way.
 */
#ifndef VK_KHR_shader_bfloat16
#define VK_KHR_SHADER_BFLOAT16_EXTENSION_NAME "VK_KHR_shader_bfloat16"
#define VK_COMPONENT_TYPE_BFLOAT16_KHR ((VkComponentTypeKHR)1000141000)
#define VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_BFLOAT16_FEATURES_KHR ((VkStructureType)1000141000)
typedef struct VkPhysicalDeviceShaderBfloat16FeaturesKHR {
	VkStructureType sType;
	void *pNext;
	VkBool32 shaderBFloat16Type;
	VkBool32 shaderBFloat16DotProduct;
	VkBool32 shaderBFloat16CooperativeMatrix;
} VkPhysicalDeviceShaderBfloat16FeaturesKHR;
#endif

#ifndef VK_EXT_shader_float8
#define VK_COMPONENT_TYPE_FLOAT8_E4M3_EXT ((VkComponentTypeKHR)1000491002)
#define VK_COMPONENT_TYPE_FLOAT8_E5M2_EXT ((VkComponentTypeKHR)1000491003)
#endif

#ifndef VK_NV_cooperative_matrix2
#define VK_NV_COOPERATIVE_MATRIX_2_EXTENSION_NAME "VK_NV_cooperative_matrix2"
#define VK_COMPONENT_TYPE_SINT8_PACKED_NV ((VkComponentTypeKHR)1000491000)
#define VK_COMPONENT_TYPE_UINT8_PACKED_NV ((VkComponentTypeKHR)1000491001)
#define VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_FLEXIBLE_DIMENSIONS_PROPERTIES_NV ((VkStructureType)1000593001)
typedef struct VkCooperativeMatrixFlexibleDimensionsPropertiesNV {
	VkStructureType sType;
	void *pNext;
	uint32_t MGranularity;
	uint32_t NGranularity;
	uint32_t KGranularity;
	VkComponentTypeKHR AType;
	VkComponentTypeKHR BType;
	VkComponentTypeKHR CType;
	VkComponentTypeKHR ResultType;
	VkBool32 saturatingAccumulation;
	VkScopeKHR scope;
	uint32_t workgroupInvocations;
} VkCooperativeMatrixFlexibleDimensionsPropertiesNV;
typedef VkResult (VKAPI_PTR *PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV)(
	VkPhysicalDevice physicalDevice, uint32_t *pPropertyCount,
	VkCooperativeMatrixFlexibleDimensionsPropertiesNV *pProperties);
#endif

/* Rotating buffers: several ctype() results are live in one printf call. */
static const char *ctype(VkComponentTypeKHR t)
{
	static char pool[8][24];
	static int slot;

	/* On an int: the fallback values above are not enumerators of an older header's type. */
	switch ((int)t) {
	case VK_COMPONENT_TYPE_FLOAT16_KHR:  return "fp16";
	case VK_COMPONENT_TYPE_FLOAT32_KHR:  return "fp32";
	case VK_COMPONENT_TYPE_FLOAT64_KHR:  return "fp64";
	case VK_COMPONENT_TYPE_BFLOAT16_KHR: return "bf16";
	case VK_COMPONENT_TYPE_SINT8_KHR:    return "sint8";
	case VK_COMPONENT_TYPE_SINT16_KHR:   return "sint16";
	case VK_COMPONENT_TYPE_SINT32_KHR:   return "sint32";
	case VK_COMPONENT_TYPE_SINT64_KHR:   return "sint64";
	case VK_COMPONENT_TYPE_UINT8_KHR:    return "uint8";
	case VK_COMPONENT_TYPE_UINT16_KHR:   return "uint16";
	case VK_COMPONENT_TYPE_UINT32_KHR:   return "uint32";
	case VK_COMPONENT_TYPE_UINT64_KHR:   return "uint64";
	case VK_COMPONENT_TYPE_FLOAT8_E4M3_EXT: return "e4m3";
	case VK_COMPONENT_TYPE_FLOAT8_E5M2_EXT: return "e5m2";
	case VK_COMPONENT_TYPE_SINT8_PACKED_NV: return "sint8p";
	case VK_COMPONENT_TYPE_UINT8_PACKED_NV: return "uint8p";
	default:
		break;
	}
	slot = (slot + 1) % 8;

#if defined(_WIN32) && __STDC_WANT_SECURE_LIB__
    sprintf_s(pool[slot], sizeof pool[slot], "enum:%d", (int)t);
#else
	snprintf(pool[slot], sizeof pool[slot], "enum:%d", (int)t);
#endif

	return pool[slot];
}

static const char *scope_name(VkScopeKHR s)
{
	switch (s) {
	case VK_SCOPE_DEVICE_KHR:       return "device";
	case VK_SCOPE_WORKGROUP_KHR:    return "workgroup";
	case VK_SCOPE_SUBGROUP_KHR:     return "subgroup";
	case VK_SCOPE_QUEUE_FAMILY_KHR: return "queuefamily";
	default:                        return "?";
	}
}

static void print_stages(VkShaderStageFlags f)
{
	static const struct { VkShaderStageFlagBits bit; const char *name; } tbl[] = {
		{ VK_SHADER_STAGE_VERTEX_BIT,   "vertex"   },
		{ VK_SHADER_STAGE_FRAGMENT_BIT, "fragment" },
		{ VK_SHADER_STAGE_COMPUTE_BIT,  "compute"  },
		{ VK_SHADER_STAGE_TASK_BIT_EXT, "task"     },
		{ VK_SHADER_STAGE_MESH_BIT_EXT, "mesh"     },
	};
	int first = 1;

	for (size_t i = 0; i < sizeof tbl / sizeof tbl[0]; i++) {
		if (f & tbl[i].bit) {
			printf("%s%s", first ? "" : "|", tbl[i].name);
			first = 0;
		}
	}
	if (first)
		printf("(none)");
}

static int has_ext(VkExtensionProperties *e, uint32_t n, const char *want)
{
	for (uint32_t i = 0; i < n; i++)
		if (!strcmp(e[i].extensionName, want))
			return 1;
	return 0;
}

int main(void)
{
	VkApplicationInfo app = { .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
				  .pApplicationName = "coopmat_probe",
				  .apiVersion = VK_API_VERSION_1_3 };
	/* macOS: MoltenVK is a portability driver, hidden by the loader until asked for. */
	const char *portability = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
	VkInstanceCreateInfo ici = { .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
				     .pApplicationInfo = &app,
#ifdef __APPLE__
				     .flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
				     .enabledExtensionCount = 1, .ppEnabledExtensionNames = &portability,
#endif
	};
	(void)portability;
	VkInstance inst;
	VkResult r = vkCreateInstance(&ici, NULL, &inst);

	if (r != VK_SUCCESS) {
		fprintf(stderr, "vkCreateInstance failed: %d\n", r);
		return 1;
	}

	PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR get_coopmat =
		(PFN_vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR)
		vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR");
	PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV get_flex =
		(PFN_vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV)
		vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceCooperativeMatrixFlexibleDimensionsPropertiesNV");

	uint32_t ndev = 0;
	vkEnumeratePhysicalDevices(inst, &ndev, NULL);
	VkPhysicalDevice *devs = calloc(ndev ? ndev : 1, sizeof *devs);
	vkEnumeratePhysicalDevices(inst, &ndev, devs);
	printf("physical devices: %u\n", ndev);

	for (uint32_t d = 0; d < ndev; d++) {
		VkPhysicalDeviceProperties2 p2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
		VkPhysicalDeviceCooperativeMatrixPropertiesKHR cmp = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_PROPERTIES_KHR };
		VkPhysicalDeviceSubgroupProperties sgp = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES, .pNext = &cmp };

		p2.pNext = &sgp;
		vkGetPhysicalDeviceProperties2(devs[d], &p2);

		uint32_t next = 0;
		vkEnumerateDeviceExtensionProperties(devs[d], NULL, &next, NULL);
		VkExtensionProperties *exts = calloc(next ? next : 1, sizeof *exts);
		vkEnumerateDeviceExtensionProperties(devs[d], NULL, &next, exts);

		int have_cm   = has_ext(exts, next, VK_KHR_COOPERATIVE_MATRIX_EXTENSION_NAME);
		int have_bf16 = has_ext(exts, next, VK_KHR_SHADER_BFLOAT16_EXTENSION_NAME);
		int have_cm2  = has_ext(exts, next, VK_NV_COOPERATIVE_MATRIX_2_EXTENSION_NAME);

		printf("\n=== device %u: %s ===\n", d, p2.properties.deviceName);
		printf("  apiVersion    : %u.%u.%u\n", VK_API_VERSION_MAJOR(p2.properties.apiVersion),
		       VK_API_VERSION_MINOR(p2.properties.apiVersion),
		       VK_API_VERSION_PATCH(p2.properties.apiVersion));
		printf("  subgroup size : %u\n", sgp.subgroupSize);
		printf("  extensions    : KHR_cooperative_matrix=%d KHR_shader_bfloat16=%d NV_cooperative_matrix2=%d\n",
		       have_cm, have_bf16, have_cm2);

		VkPhysicalDeviceFeatures2 f2 = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
		VkPhysicalDeviceCooperativeMatrixFeaturesKHR cmf = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_COOPERATIVE_MATRIX_FEATURES_KHR };
		VkPhysicalDeviceShaderBfloat16FeaturesKHR bff = {
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_BFLOAT16_FEATURES_KHR };

		f2.pNext = &cmf;
		if (have_bf16)
			cmf.pNext = &bff;
		vkGetPhysicalDeviceFeatures2(devs[d], &f2);

		printf("  cooperativeMatrix              = %u\n", cmf.cooperativeMatrix);
		printf("  cooperativeMatrixRobustBufferAccess = %u\n", cmf.cooperativeMatrixRobustBufferAccess);
		printf("  supportedStages                = ");
		print_stages(cmp.cooperativeMatrixSupportedStages);
		printf("\n");
		if (have_bf16) {
			printf("  shaderBFloat16Type             = %u\n", bff.shaderBFloat16Type);
			printf("  shaderBFloat16CooperativeMatrix= %u\n", bff.shaderBFloat16CooperativeMatrix);
			printf("  shaderBFloat16DotProduct       = %u\n", bff.shaderBFloat16DotProduct);
		}

		if (!have_cm || !get_coopmat) {
			printf("  -- no KHR cooperative matrix here --\n");
			free(exts);
			continue;
		}

		uint32_t n = 0;
		r = get_coopmat(devs[d], &n, NULL);
		printf("\n  vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR -> %d, count=%u\n", r, n);
		if (r == VK_SUCCESS && n) {
			VkCooperativeMatrixPropertiesKHR *props = calloc(n, sizeof *props);
			for (uint32_t i = 0; i < n; i++)
				props[i].sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_PROPERTIES_KHR;
			r = get_coopmat(devs[d], &n, props);

			printf("\n   idx   M   N   K   A      B      C      Result  satAcc  scope\n");
			printf("   ---------------------------------------------------------------\n");
			for (uint32_t i = 0; i < n; i++)
				printf("   %3u %3u %3u %3u   %-6s %-6s %-6s %-6s  %-6u %s\n", i,
				       props[i].MSize, props[i].NSize, props[i].KSize,
				       ctype(props[i].AType), ctype(props[i].BType),
				       ctype(props[i].CType), ctype(props[i].ResultType),
				       props[i].saturatingAccumulation, scope_name(props[i].scope));

			/* Open question 3: FP32 accumulation for BF16 inputs. */
			unsigned bf16 = 0, bf16_fp32acc = 0, fp16_fp32acc = 0;
			for (uint32_t i = 0; i < n; i++) {
				if (props[i].AType == VK_COMPONENT_TYPE_BFLOAT16_KHR) {
					bf16++;
					if (props[i].CType == VK_COMPONENT_TYPE_FLOAT32_KHR &&
					    props[i].ResultType == VK_COMPONENT_TYPE_FLOAT32_KHR)
						bf16_fp32acc++;
				}
				if (props[i].AType == VK_COMPONENT_TYPE_FLOAT16_KHR &&
				    props[i].CType == VK_COMPONENT_TYPE_FLOAT32_KHR)
					fp16_fp32acc++;
			}
			printf("\n   summary: %u configs total; %u with A=bf16, of those %u accumulate in fp32; "
			       "%u fp16 configs accumulate in fp32\n", n, bf16, bf16_fp32acc, fp16_fp32acc);
			free(props);
		}

		if (have_cm2 && get_flex) {
			uint32_t nf = 0;
			r = get_flex(devs[d], &nf, NULL);
			printf("\n  NV_cooperative_matrix2 flexible dimensions -> %d, count=%u\n", r, nf);
			if (r == VK_SUCCESS && nf) {
				VkCooperativeMatrixFlexibleDimensionsPropertiesNV *fp = calloc(nf, sizeof *fp);
				for (uint32_t i = 0; i < nf; i++)
					fp[i].sType = VK_STRUCTURE_TYPE_COOPERATIVE_MATRIX_FLEXIBLE_DIMENSIONS_PROPERTIES_NV;
				get_flex(devs[d], &nf, fp);
				printf("\n   idx  Mgran Ngran Kgran  A      B      C      Result  scope    wgsize saturate\n");
				printf("   ------------------------------------------------------------------------------\n");
				for (uint32_t i = 0; i < nf; i++)
					printf("   %3u  %5u %5u %5u  %-6s %-6s %-6s %-6s  %-8s %-6u %u\n", i,
					       fp[i].MGranularity, fp[i].NGranularity, fp[i].KGranularity,
					       ctype(fp[i].AType), ctype(fp[i].BType), ctype(fp[i].CType),
					       ctype(fp[i].ResultType), scope_name(fp[i].scope),
					       fp[i].workgroupInvocations, fp[i].saturatingAccumulation);
				free(fp);
			}
		}
		free(exts);
	}
	vkDestroyInstance(inst, NULL);
	return 0;
}
