
#include <stdlib.h>
#include <stdio.h>
#include "../DuckLib/core.h"
#include "../duckLisp.h"

dl_error_t duckLispDev_callback_printString(duckLisp_t *duckLisp) {
	dl_error_t e = dl_error_ok;
	
	duckLisp_object_t string;
	
	e = duckLisp_getArg(duckLisp, &string, 1);
	if (e) {
		goto l_cleanup;
	}
	
	if (string.type != duckLisp_object_type_string) {
		e = duckLisp_error_pushRuntime(duckLisp, DL_STR("Argument should be a string."));
		e = e ? e : dl_error_invalidValue;
		goto l_cleanup;
	}
	
	for (dl_size_t i = 0; i < string.value.string.value_length; i++) {
		putchar(string.value.string.value[i]);
	}
	
	e = duckLisp_pushReturn(duckLisp, string);
	
	l_cleanup:
	
	return e;
}

/*
dl_error_t duckLisp_generator_functionCall(duckLisp_t *duckLisp) {
	dl_error_t e = dl_error_ok;
	
	// 
	// Insert arg1 into this scope's name trie.
	// Set object to type string.
	// Set object to value of arg2.
	
	l_cleanup:
	
	return e;
}
*/

dl_error_t duckLispDev_generator_createString(duckLisp_t *duckLisp, dl_array_t *assembly, duckLisp_ast_expression_t *expression) {
	dl_error_t e = dl_error_ok;
	dl_error_t eError = dl_error_ok;
	dl_array_t eString;
	/**/ dl_array_init(&eString, &duckLisp->memoryAllocation, sizeof(char), dl_array_strategy_double);
	
	dl_ptrdiff_t identifier_index = -1;
	dl_ptrdiff_t string_index = -1;
	dl_array_t *assemblyFragment = dl_null;
	
	/* Check arguments for call and type errors. */
	
	e = duckLisp_checkArgsAndReportError(duckLisp, *expression, 3);
	if (e) {
		goto l_cleanup;
	}
	
	if (expression->compoundExpressions[1].type != ast_compoundExpression_type_identifier) {
		e = dl_array_pushElements(&eString, DL_STR("Argument 1 of function \""));
		if (e) {
			goto l_cleanup;
		}
		e = dl_array_pushElements(&eString, expression->compoundExpressions[0].value.identifier.value,
		                          expression->compoundExpressions[0].value.identifier.value_length);
		if (e) {
			goto l_cleanup;
		}
		e = dl_array_pushElements(&eString, DL_STR("\" should be an identifier."));
		if (e) {
			goto l_cleanup;
		}
		eError = duckLisp_error_pushRuntime(duckLisp, eString.elements, eString.elements_length * eString.element_size);
		if (eError) {
			e = eError;
		}
		goto l_cleanup;
	}
	
	if ((expression->compoundExpressions[2].type != ast_compoundExpression_type_constant) &&
	    (expression->compoundExpressions[2].value.constant.type != ast_constant_type_string)) {
		e = dl_array_pushElements(&eString, DL_STR("Argument 2 of function \""));
		if (e) {
			goto l_cleanup;
		}
		e = dl_array_pushElements(&eString, expression->compoundExpressions[0].value.identifier.value,
		                          expression->compoundExpressions[0].value.identifier.value_length);
		if (e) {
			goto l_cleanup;
		}
		e = dl_array_pushElements(&eString, DL_STR("\" should be a string."));
		if (e) {
			goto l_cleanup;
		}
		eError = duckLisp_error_pushRuntime(duckLisp, eString.elements, eString.elements_length * eString.element_size);
		if (eError) {
			e = eError;
		}
		goto l_cleanup;
	}
	
	// Create the string variable.
	e = duckLisp_emit_pushString(duckLisp, assembly, &identifier_index, expression->compoundExpressions[2].value.constant.value.string.value,
	                                 expression->compoundExpressions[2].value.constant.value.string.value_length);
	if (e) {
		goto l_cleanup;
	}
	
	// Insert arg1 into this scope's name trie.
	e = duckLisp_scope_addObject(duckLisp, expression->compoundExpressions[1].value.constant.value.string.value,
	                             expression->compoundExpressions[1].value.constant.value.string.value_length);
	if (e) {
		goto l_cleanup;
	}
	
	l_cleanup:
	
	eError = dl_array_quit(&eString);
	if (eError) {
		e = eError;
	}
	
	return e;
}



int main(int argc, char *argv[]) {
	dl_error_t e = dl_error_ok;
	struct {
		dl_bool_t duckLispMemory;
		dl_bool_t duckLisp_init;
	} d = {0};
	
	duckLisp_t duckLisp;
	void *duckLispMemory = dl_null;
	size_t tempMemory_size;
	duckLisp_error_t error;
	char *scriptHandles[3] = {
		"hello-world"
	};
	size_t scriptHandles_lengths[3] = {
		11
	};
	const char source0[] = "((string s \"Hello, world!\") (print-string s))";
	// const char source0[] = "(print-string (print-string (string s \"Hello, world!\")))";
	// const char source0[] = "((string s7 \"7\") (print-string s7) ((string s3 \"3\") (print-string s3) ((string s1 \"1\") (print-string s1)) ((string s2 \"2\") (print-string s2))) ((string s6 \"6\") (print-string s6) ((string s4 \"4\") (print-string s4)) ((string s5 \"5\") (print-string s5))))";
	// const char source0[] = "((string s7 \"7\") (print-string s7) ((string s3 \"3\") (print-string s3) ((string s1 \"1\") (print-string s1)) ((string s2 \"2\") (print-string s2))) ((string s6 \"6\") (print-string s6) ((string s4 \"4\") (print-string s4)) ((string s5 \"5\") (print-string s5))))";
	// const char source0[] = "(((string s1 \"1\") (string s2 \"2\")) ((string s3 \"3\") (string s4 \"4\")))";
	// const char source0[] = "(print-string 7 (print-string 3 (print-string 1)) (print-string 6 (print-string 4) (print-string 5)))";
	// const char source1[] = "((int i -5) (bool b true) (bool b false) (print i))";
	// const char source2[] = "((float f 1.4e656) (float f0 -1.4e656) (float f1 .4e6) (float f2 1.4e-656) (float f3 -.4e6) (float f3 -10.e-2) (echo #float) (print f))";
	duckLisp_object_t tempObject;
	dl_ptrdiff_t printString_index = -1;
	
	/* Initialization. */
	
	tempMemory_size = 1024*1024;
	duckLispMemory = malloc(tempMemory_size);
	if (duckLispMemory == NULL) {
		e = dl_error_outOfMemory;
		printf("Out of memory.\n");
		goto l_cleanup;
	}
	d.duckLispMemory = dl_true;
	
	e = duckLisp_init(&duckLisp, duckLispMemory, tempMemory_size);
	if (e) {
		printf("Could not initialize DuckLisp. (%s)\n", dl_errorString[e]);
		goto l_cleanup;
	}
	d.duckLisp_init = dl_true;
	
	/* Create generators. */
	
	// e = duckLisp_pushGenerator(&duckLisp, DL_STR("string"), duckLispDev_generator_createString);
	e = duckLisp_addGenerator(&duckLisp, duckLispDev_generator_createString, DL_STR("string"));
	if (e) {
		printf("Could not register generator. (%s)\n", dl_errorString[e]);
	}
	
	/* Add C functions. */
	
	// tempObject.type = duckLisp_object_type_function;
	// tempObject.value.function.callback = duckLispDev_callback_printString;
	e = duckLisp_linkCFunction(&duckLisp, &printString_index, DL_STR("print-string"));
	if (e) {
		printf("Could not create function. (%s)\n", dl_errorString[e]);
		goto l_cleanup;
	}
	
	/* Compile functions. */
	
	e = duckLisp_loadString(&duckLisp, scriptHandles[0], scriptHandles_lengths[0], DL_STR(source0));
	if (e) {
		printf("Error loading string. (%s)\n", dl_errorString[e]);
		
		goto l_cleanup;
	}
	
	e = dl_memory_checkHealth(duckLisp.memoryAllocation);
	if (e) {
		printf("Memory health check failed. (%s)\n", dl_errorString[e]);
	}
	
	while (dl_true) {
		e = dl_array_popElement(&duckLisp.errors, (void *) &error);
		if (e) {
			break;
		}
		
		// for (dl_ptrdiff_t i = 0; i < duckLisp.source.elements_length; i++) {
		// 	putchar(((char *) duckLisp.source.elements)[i]);
		// }
		// putchar('\n');
		
		for (dl_ptrdiff_t i = 0; i < error.message_length; i++) {
			putchar(error.message[i]);
		}
		putchar('\n');
		
		if (error.index == -1) {
			continue;
		}
		
		printf("%s\n", source0);
		for (dl_ptrdiff_t i = duckLisp.source.elements_length - sizeof(source0); i < error.index; i++) {
			putchar(' ');
		}
		puts("^");
	}
	
	puts("Scope 0: variables");
	/**/ dl_trie_print_compact(((duckLisp_scope_t *) duckLisp.scope_stack.elements)[0].variables_trie);
	puts("Scope 0: generators");
	/**/ dl_trie_print_compact(((duckLisp_scope_t *) duckLisp.scope_stack.elements)[0].generators_trie);
	puts("Scope 0: functions (1: callback  2: script  3: generator)");
	/**/ dl_trie_print_compact(((duckLisp_scope_t *) duckLisp.scope_stack.elements)[0].functions_trie);
	
	
	// /**/ duckLisp_call(&duckLisp, "hello-world");
	
	/* Execute. */
	
	
	l_cleanup:
	
	if (d.duckLisp_init) {
		puts("");
		printf("Memory in use:   %llu\n", duckLisp.memoryAllocation.used);
		printf("Max memory used: %llu\n", duckLisp.memoryAllocation.max_used);
		// dl_memory_printMemoryAllocation(duckLisp.memoryAllocation);
		// Clear 1 MiB of memory. Is surprisingly fast. I'm mainly did this to see how simple it was.
		/**/ dl_memclear(duckLisp.memoryAllocation.memory, duckLisp.memoryAllocation.size * sizeof(char));
		/**/ duckLisp_quit(&duckLisp);
	}
	if (d.duckLispMemory) {
		/**/ free(duckLispMemory); duckLispMemory = NULL;
	}
	
	return e;
}
