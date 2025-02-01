

#include <cstdio>


namespace ErrorCheck
{
	
#define VULKAN_CHECK(l_vkfunc)																						\
		{																												\
			const VkResult lv_result = l_vkfunc;																		\
			if (lv_result != VK_SUCCESS && lv_result != VK_TIMEOUT) {										\
				printf("\n\n******** Error info ********\n\n");					 																\
				printf("Error calling function: %s\nAt File: %s \nAt line number: %d\n",\
			#l_vkfunc, __FILE__, __LINE__);													\
				exit(EXIT_FAILURE);																							\
			}																											\
		}

#define PRINT_EXIT(l_expressionToPrint)\
		{\
			printf(l_expressionToPrint);	\
			exit(EXIT_FAILURE);\
		}


}