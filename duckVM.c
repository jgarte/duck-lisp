
#include "duckVM.h"
#include <stdio.h>

dl_error_t duckVM_init(duckVM_t *duckVM, void *memory, dl_size_t size) {
	dl_error_t e = dl_error_ok;

	e = dl_memory_init(&duckVM->memoryAllocation, memory, size, dl_memoryFit_best);
	if (e) {
		goto l_cleanup;
	}

	/**/ dl_array_init(&duckVM->errors, &duckVM->memoryAllocation, sizeof(duckLisp_error_t), dl_array_strategy_fit);
	/**/ dl_array_init(&duckVM->stack, &duckVM->memoryAllocation, sizeof(duckLisp_object_t), dl_array_strategy_fit);
	/**/ dl_array_init(&duckVM->statics, &duckVM->memoryAllocation, sizeof(duckLisp_object_t), dl_array_strategy_fit);
	duckVM->frame_pointer = 0;

	l_cleanup:

	return e;
}

void duckVM_quit(duckVM_t *duckVM) {
	/**/ dl_memory_quit(&duckVM->memoryAllocation);
	/**/ dl_memclear(&duckVM->errors, sizeof(dl_array_t));
	/**/ dl_memclear(&duckVM->statics, sizeof(dl_array_t));
	/**/ dl_memclear(&duckVM->stack, sizeof(dl_array_t));
	duckVM->frame_pointer = -1;
}

dl_error_t duckVM_execute(duckVM_t *duckVM, unsigned char *bytecode) {
	dl_error_t e = dl_error_ok;
	
	unsigned char *ip = bytecode;
	dl_ptrdiff_t ptrdiff1;
	// dl_size_t length1;
	duckLisp_object_t object1;
	unsigned char *pointer1;
	/**/ dl_memclear(&object1, sizeof(duckLisp_object_t));
	
	do {
		ptrdiff1 = 0;
		/**/ dl_memclear(&object1, sizeof(duckLisp_object_t));
		switch (*(ip++)) {
		case duckLisp_instruction_pushString32:
			object1.value.string.value_length = *(ip)++;
			object1.value.string.value_length = *(ip++) + (object1.value.string.value_length << 8);
		case duckLisp_instruction_pushString16:
			object1.value.string.value_length = *(ip++) + (object1.value.string.value_length << 8);
		case duckLisp_instruction_pushString8:
			object1.value.string.value_length = *(ip++) + (object1.value.string.value_length << 8);
			object1.value.string.value = ip;
			ip += object1.value.string.value_length;
			object1.type = duckLisp_object_type_string;
			e = dl_array_pushElement(&duckVM->stack, &object1);
			break;

		case duckLisp_instruction_pushInteger32:
		// case duckLisp_instruction_pushIndex32:
			object1.value.integer = *(ip++);
			object1.value.integer = *(ip++) + (object1.value.integer << 8);
		case duckLisp_instruction_pushInteger16:
		// case duckLisp_instruction_pushIndex16:
			object1.value.integer = *(ip++) + (object1.value.integer << 8);
		case duckLisp_instruction_pushInteger8:
		// case duckLisp_instruction_pushIndex8:
			object1.value.integer = *(ip++) + (object1.value.integer << 8);
			object1.type = duckLisp_object_type_integer;
			e = dl_array_pushElement(&duckVM->stack, &object1);
			break;

		case duckLisp_instruction_pushIndex32:
			ptrdiff1 = *(ip++);
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
		case duckLisp_instruction_pushIndex16:
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
		case duckLisp_instruction_pushIndex8:
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);// + duckVM->frame_pointer;
			e = dl_array_get(&duckVM->stack, &object1, ptrdiff1);
			if (e) {
				break;
			}
			e = dl_array_pushElement(&duckVM->stack, &object1);
			break;

		case duckLisp_instruction_call32:
			ptrdiff1 = *(ip++);
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
		case duckLisp_instruction_call16:
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
		case duckLisp_instruction_call8:
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
			e = duckVM_execute(duckVM, DL_ARRAY_GETADDRESS(duckVM->statics, duckLisp_object_t, ptrdiff1).value.function.bytecode);
			break;

		case duckLisp_instruction_ccall32:
			ptrdiff1 = *(ip++);
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
		case duckLisp_instruction_ccall16:
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
		case duckLisp_instruction_ccall8:
			ptrdiff1 = *(ip++) + (ptrdiff1 << 8);
			e = DL_ARRAY_GETADDRESS(duckVM->statics, duckLisp_object_t, ptrdiff1).value.function.callback(duckVM);
			break;
		
		case duckLisp_instruction_return:
			goto l_cleanup;

		default:
			e = dl_error_invalidValue;
		}
	} while (!e);
	
	l_cleanup:
	
	return e;
}

dl_error_t duckVM_callLocal(duckVM_t *duckVM, dl_ptrdiff_t function_index) {
	dl_error_t e = dl_error_ok;
	
	duckLisp_object_t functionObject;
	
	// Allow addressing relative to the top of the stack with negative indices.
	if (function_index < 0) {
		function_index = duckVM->stack.elements_length + function_index;
	}
	if ((function_index < 0) || (function_index  >= duckVM->stack.elements_length)) {
		e = dl_error_invalidValue;
		goto l_cleanup;
	}
	
	functionObject = DL_ARRAY_GETADDRESS(duckVM->stack, duckLisp_object_t, function_index);
	if (functionObject.type != duckLisp_object_type_function) {
		// Push runtime error.
		e = dl_error_invalidValue;
		goto l_cleanup;
	}
	
	if (functionObject.value.function.callback != dl_null) {
		e = functionObject.value.function.callback(duckVM);
	}
	else if (functionObject.value.function.bytecode != dl_null) {
		e = duckVM_execute(duckVM, functionObject.value.function.bytecode);
	}
	else {
		e = dl_error_invalidValue;
	}
	
	l_cleanup:
	
	return e;
}

dl_error_t duckVM_linkCFunction(duckVM_t *duckVM, dl_ptrdiff_t callback_index, dl_error_t (*callback)(duckVM_t *)) {
	dl_error_t e = dl_error_ok;

	duckLisp_object_t object;
	dl_memclear(&object, sizeof(duckLisp_object_t));
	object.type = duckLisp_object_type_function;
	object.value.function.callback = callback;

	// Make room for the object if the index reaches beyond the end.
	if (callback_index >= duckVM->statics.elements_length) {
		e = dl_array_pushElements(&duckVM->statics, dl_null, 1 + callback_index - duckVM->statics.elements_length);
		if (e) {
			goto l_cleanup;
		}
	}
	
	// Insert the callback.
	DL_ARRAY_GETADDRESS(duckVM->statics, duckLisp_object_t, callback_index) = object;

	l_cleanup:

	return e;
}

///////////////////////////////////////
// Functions for C callbacks to use. //
///////////////////////////////////////

void duckVM_getArgLength(duckVM_t *duckVM, dl_size_t *length) {
	*length = DL_ARRAY_GETADDRESS(duckVM->stack, duckLisp_object_t, duckVM->frame_pointer).value.integer;
}

dl_error_t duckVM_getArg(duckVM_t *duckVM, duckLisp_object_t *object, dl_ptrdiff_t index) {
	if (index < DL_ARRAY_GETADDRESS(duckVM->stack, duckLisp_object_t, duckVM->frame_pointer).value.integer) {
		*object = DL_ARRAY_GETADDRESS(duckVM->stack, duckLisp_object_t, duckVM->frame_pointer + index);
		return dl_error_ok;
	}
	else {
		return dl_error_bufferOverflow;
	}
}

dl_error_t duckVM_pop(duckVM_t *duckVM, duckLisp_object_t *object) {
	return dl_array_popElement(&duckVM->stack, object);
}

dl_error_t duckVM_pushReturn(duckVM_t *duckVM, duckLisp_object_t object) {
	dl_error_t e = dl_error_ok;
	
	e = dl_array_pushElement(&duckVM->stack, (void *) &object);
	duckVM->frame_pointer++;
	
	return e;
}
