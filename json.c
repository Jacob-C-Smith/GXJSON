/** !
 * JSON parser / serializer
 * 
 * @file json.c
 * 
 * @author Jacob Smith
 */

// Header 
#include <json/json.h>

// Data
const size_t DICT_SIZE = 16;
static bool initialized = false;

void json_init ( void )
{

    // State check
    if ( initialized == true ) return;

    // Initialize the log library
    log_init();

    // Initialize the sync library
    sync_init();

    // Initialize the hash cache library
    hash_cache_init();

    // Initialize the dict library
    dict_init();

    // Initialize the array library
    array_init();

    // Set the initialized flag
    initialized = true;

    // Done
    return;
}

int json_whitespace_parse ( char *pointer, char **return_pointer )
{

    // Argument check
    if (  pointer == (void *) 0 ) goto no_pointer;
    if ( *pointer ==       '\0' ) goto done;

    // Skip past spaces, line feed, carriage return, horizontal tab
    while (
        *pointer == ' '  ||
        *pointer == '\n' ||
        *pointer == '\r' ||
        *pointer == '\t' ||
        *pointer == '\0' 
    )
    { pointer++; };

    // Error checking
    if ( *pointer == '\0' ) return 0;
    
    done:

    // Update the pointer
    *return_pointer = pointer;

    // Success
    return 1;

    // Error handling
    {

        // Argument errors
        {
            no_pointer:
                #ifndef NDEBUG
                    log_error("[json] Null pointer provided for parameter \"pointer\" in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;

        }
    }
}

int json_string_parse ( char *const pointer, char **return_pointer )
{

    // Argument check
    if ( pointer == (void *) 0 ) goto no_pointer;

    // Initialized data
    size_t i = 0,
           j = 1;

    // Error check
    if ( pointer[i] != '\"' ) return 0;
    
    // Check for an empty string
    if ( pointer[0] == '\0' ) goto exit;

    // Walk the string
    while ( pointer[j] )
    {

        // Continuation condition
        if ( pointer[j] == '\"' ) goto exit;
        
        // Escape sequence
        else if ( pointer[j] == '\\' )
        {

            // Escape sequence strategy
            switch ( pointer[j+1] )
            {

                // Single quote
                case '\"':

                    // Write the carriage return 
                    pointer[i] = '\"';

                    // Done
                    break;

                // Backslash
                case '\\':

                    // Write the backslash 
                    pointer[i] = '\\';

                    // Done
                    break;

                // Forward slash
                case '/':

                    // Write the forward slash 
                    pointer[i] = '/';

                    // Done
                    break;

                // Backspace
                case 'b':

                    // Write the backspace 
                    pointer[i] = '\b';

                    // Done
                    break;

                // Form feed
                case 'f':

                    // Write the form feed  
                    pointer[i] = '\f';

                    // Done 
                    break;

                // Line feed
                case 'n':

                    // Write the line feed 
                    pointer[i] = '\n';

                    // Done
                    break;
                
                // Carriage return
                case 'r':

                    // Write the carriage return 
                    pointer[i] = '\r';

                    break;

                // Horizontal tab
                case 't':

                    // Write the horizontal tab 
                    pointer[i]='\t';

                    // Done
                    break;
                
                // TODO: Unicode
                case 'u':

                    // TODO:

                    // Done
                    break;

                // Bad string
                case '\0':
                default:

                    // Error handling
                    goto unexpected_escape_sequence;
            }
            
            // Increment the cursor
            j++;
        }

        // Copy the text
        else
            pointer[i] = pointer[j];
        
        // Increment the cursors
        i++, j++;
    }
    
    exit:

    // Insert a null terminator
    pointer[i] = '\0';

    // Return the updated pointer
    *return_pointer = &pointer[j];

    // Success
    return 1;
    
    // Error handling
    {

        // Argument errors
        {
            no_pointer:
                #ifndef NDEBUG
                    log_error("[json] Null pointer provided for parameter \"pointer\" in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }

        // json errors
        {
            unexpected_escape_sequence:
                #ifndef NDEBUG
                    log_error("[json] Unexpected escape sequence while parsing string in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

int json_object_parse ( char *pointer, char **return_pointer, dict **const pp_dict )
{
    
    // Initialized data
    dict *p_dict = (void *)0;

    // Construct a dict
    if ( dict_construct(&p_dict, DICT_SIZE, 0) == 0 ) goto failed_to_construct_dict;

    // Error checking
    if ( *pointer != '{' ) return 0;

    // Increment the cursor
    pointer++;

    parse_property:

    // Parse whitespaces
    json_whitespace_parse(pointer, &pointer);

    // Parse the property key
    if ( *pointer == '\"' )
    {

        // Initialized data
        char       *key   = pointer;
        json_value *value = 0;

        // Parse the property key
        json_string_parse(pointer, &pointer);

        // Increment the cursor
        pointer++;

        // Parse any whitespace
        json_whitespace_parse(pointer, &pointer);

        // Check for a valid property
        if ( *pointer == ':' ) pointer++;

        // Invalid property
        else goto expected_value;

        // Parse a JSON value
        if ( json_value_parse(pointer, &pointer, &value) == 0 ) goto failed_to_parse_json_value;

        // Add the value to the dictionary
        dict_add(p_dict, key, value);

        // Search for the next property ...
        if ( *pointer == ',' )
        {

            // Increment the property counter and the pointer 
            pointer++;

            // Parse another property
            goto parse_property;
        }

        // ... or the end of the object
        else if ( *pointer == '}' );
        
        // Default
        else

            // Error
            return 0;
    }
    
    // Error checking
    if ( *pointer != '}' ) return 0;

    // Increment the cursor
    pointer++;

    // Return the cursor to the caller
    if ( return_pointer ) *return_pointer = pointer;

    // Return a pointer to the caller
    *pp_dict = p_dict;

    // Success
    return 1;

    // Error check
    {
        
        // dict errors
        {
            failed_to_construct_dict:
                #ifndef NDEBUG
                    log_error("[dict] Dictionary constructor returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }

        // json errors
        {
            failed_to_parse_json_value:
                #ifndef NDEBUG
                    log_error("[json] Failed to parse json value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;

            expected_value:
                #ifndef NDEBUG
                    log_error("[json] Expected colon delimiter while parsing json text in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

int json_array_parse ( char *pointer, char **return_pointer, array **const pp_array )
{

    // Initialized data
    size_t  i       = 0;
    array  *p_array = 0;

    // Construct an array
    if ( array_construct(&p_array, DICT_SIZE) == 0 ) goto failed_to_construct_array;

    // Error checking
    if ( pointer[0] != '[' ) return 0;

    // Increment the cursor
    pointer++;

    parse_property:
    
    // Parse whitespaces
    json_whitespace_parse(pointer, &pointer);

    // Walk the text
    if ( pointer[0] != ']' )
    {

        // Initialized data
        json_value *value = 0;

        // Parse the text into a value
        json_value_parse(&pointer[i], &pointer, &value);

        // Add the value to the array
        array_add(p_array, value);

        // Check for another property
        if ( *pointer == ',' )
        {

            // Increment the property counter and the cursor
            pointer++;

            // Parse another property
            goto parse_property;
        }

        // Terminate
        else if ( *pointer == ']' );

        // Default
        else

            // Error
            return 0;
    }
    
    // Error checking
    if ( *pointer != ']' ) return 0;
        
    // Increment the cursor
    pointer++;

    // Return the cursor to the caller
    if ( return_pointer ) *return_pointer = pointer;

    // Return a pointer to the caller
    *pp_array = p_array;

    // Success
    return 1;

    // Error check
    {
        
        // dict errors
        {
            failed_to_construct_array:
                #ifndef NDEBUG
                    log_error("[array] Array constructor returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

int json_value_parse ( char *text, char **return_pointer, json_value **const pp_value )
{

    // Argument check
    if ( text     == (void *) 0 ) goto no_text;
    if ( pp_value == (void *) 0 ) goto no_value;

    // Initialized data
    json_value *p_value = JSON_REALLOC(0, sizeof(json_value));

    // Parse whitespace
    (void) json_whitespace_parse(text, &text);

    // Parse a value
    switch ( *text )
    {

        // This branch parses a string
        case '\"':
        {

            // Initialized data
            char *last_text = text;
            size_t string_len = 0;
            
            // Parse the string
            if ( json_string_parse(text, &text) == 0 ) goto failed_to_parse_json_string;
            
            // Increment the cursor
            text++;

            // Compute the length of the string
            string_len = strlen(last_text); 

            // Allocate memory for the string
            p_value->string = JSON_REALLOC(0, ( (string_len + 1 + 8) & 0xFFFFFFFFFFFFFFF8) * sizeof(char));
        
            // Error check
            if ( p_value->string == (void *) 0 ) goto no_mem;

            // Initialize memory
            memset(p_value->string, 0, ( (string_len + 1 + 8) & 0xFFFFFFFFFFFFFFF8) * sizeof(char));

            // Copy the string 
            (void) strncpy(p_value->string, last_text, string_len+1);

            // Set the return type
            p_value->type = JSON_VALUE_STRING;
            
            // Done
            break;
        }
        
        // This branch parses an object
        case '{':
            
            // Parse the JSON text as an object
            if ( json_object_parse(text, &text, &p_value->object) == 0 ) goto failed_to_parse_json_object;

            // Set the value type
            p_value->type = JSON_VALUE_OBJECT;
        
            // Done
            break;
        
        
        // This branch parses an array
        case '[':

            // Parse the JSON text as an array
            if ( json_array_parse(text, &text, &p_value->list) == 0 ) goto failed_to_parse_json_array;

            // Set the value type
            p_value->type = JSON_VALUE_ARRAY;

            // Done
            break;

        // This branch parses the 'true' keyword
        case 't':

            // Check for the correct keyworkd
            if ( strncmp(text, "true", 4) ) goto failed_to_parse_keyword;

            // Store a true value
            *p_value = (json_value)
            {
                .type = JSON_VALUE_BOOLEAN,
                .boolean = true
            };

            // Skip the cursor
            text += 4;

            // Done
            break;
        
        // This branch parses the 'false' keyword
        case 'f':
            
            // Check for the 'false' keyword
            if ( strncmp(text, "false", 5) ) goto failed_to_parse_keyword;

            // Store a true value
            *p_value = (json_value)
            {
                .type = JSON_VALUE_BOOLEAN,
                .boolean = false
            };

            // Skip the cursor
            text+=5;
            
            // Done
            break;
        
        // This branch parses the 'null' keyword
        case 'n':

            // Error check
            if ( strncmp(text, "null" , 4) ) goto failed_to_parse_keyword;
        
            // Free the JSON value
            if ( JSON_REALLOC(p_value, 0) ) goto failed_to_free;
            
            // Skip the cursor
            text += 4;

            // p_value = nullptr
            p_value = 0;
        
            // Done
            break;

        // Parse a number
        default:

            // Done
            break;
    }

    // Parse a number
    if ( *text == '-' || (*text >= '0' && *text <= '9') ) 
    {
        // Initialized data
        size_t i = 0,
               d = 0;
        bool   f = false;
        bool   n = false;
        
        // Check the number's sign
        if ( text[0] == '-' )

            // Set the negative bit, and skip past the sign
            i++, n = true;

        // Skip digits and decimal places
        while ( ( text[i] >= '0' && text[i] <= '9' ) || text[i] == '.' ) 
        { 

            // Check for a float
            f |= ( text[i] == '.' );
            
            // Increment
            i++, d++;
        };

        // Clear errors
        errno = 0;

        // Parse a floating point
        if ( f )
        {

            // Clear error
            errno = 0;

            // Store a number value
            *p_value = (json_value)
            {
                .type = JSON_VALUE_NUMBER,
                .number = strtod(text, NULL)
            };

            // Bounds check
            if ( errno == ERANGE )
            {
                
                // Free the JSON value
                if ( JSON_REALLOC(p_value, 0) ) goto failed_to_free;

                // Error handling
                goto float_bounds_exceeded;
            }
        }

        // Parse an integer
        else
        {
 
            // Clear error
            errno = 0;

            // Set the type of the value
            *p_value = (json_value)
            {
                .type = JSON_VALUE_INTEGER,
                .integer = strtoll(text, NULL, 10)
            };

            // Bounds check
            if ( errno == ERANGE )
            {

                // Free the value
                if ( JSON_REALLOC(p_value, 0) ) goto failed_to_free;

                // Error handling
                goto integer_bounds_exceeded;
            }
        }

        // Decrement the decimal places
        d--;

        // Skip N decimal places
        text+=d; 

        // Increment the cursor
        text++;

        // Increment the cursor again if the number has a - sign
        text += n ? 1 : 0;
    }

    // Parse whitespace
    (void) json_whitespace_parse(text, &text);

    // Write the return value
    if ( pp_value ) *pp_value = p_value;

    // Update the cursor
    if ( return_pointer ) *return_pointer = text;

    // Success
    return 1;

    // Error handling
    {

        // Argument errors
        {
            no_text:
                #ifndef NDEBUG
                    log_error("[json] Null pointer provided for parameter \"text\" in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;

            no_value:
                #ifndef NDEBUG
                    log_error("[json] Null pointer provided for parameter \"pp_value\" in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }

        // json errors
        {
            failed_to_parse_json_string:
                #ifndef NDEBUG
                    log_error("[json] Failed to parse json string in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error 
                return 0;
                
            failed_to_parse_json_array:
                #ifndef NDEBUG
                    log_error("[json] Failed to parse json array in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error 
                return 0;
                
            failed_to_parse_json_object:
                #ifndef NDEBUG
                    log_error("[json] Failed to parse json object in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error 
                return 0;

            failed_to_parse_keyword:
                #ifndef NDEBUG
                    log_error("[json] Failed to parse json keyword in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Clean up
                if ( JSON_REALLOC(p_value, 0) ) goto failed_to_free;

                // Error
                return 0;

            integer_bounds_exceeded:
                #ifndef NDEBUG

                    // For some reason, printing this on one line caused a segfault. 
                    // Maybe I should report it?
                    log_error("[json] Integer must be between [%lld, ", -9223372036854775807LL);
                    log_error("%lld] in call to function \"%s\"\n",9223372036854775807LL, __FUNCTION__);
                #endif


                // Error
                return 0;
            
            float_bounds_exceeded:
                #ifndef NDEBUG

                    // For some reason, printing this on one line caused a segfault. 
                    // Maybe I should report it?
                    log_error("[json] Float must be between [%lg, ", -DBL_MAX);
                    log_error("%lg] in call to function \"%s\"\n",DBL_MAX, __FUNCTION__);
                #endif


                // Error
                return 0;
        }

        // Standard library errors
        {
            no_mem:
                #ifndef NDEBUG
					printf("[Standard Library] Failed to allocate memory in call to function \"%s\"\n", __FUNCTION__);
				#endif

				// Error
				return 0;
                
            failed_to_free:
                #ifndef NDEBUG
                    printf("[Standard Library] Call to \"realloc\" returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

int json_value_serialize ( const json_value *const p_value, char *_buffer )
{

    size_t written_characters = 0;

    // Null case
    if ( p_value == 0 ) written_characters += (size_t) sprintf(&_buffer[written_characters], "null");

    // Everything else
    else
    {

        // Print the value
        switch ( p_value->type )
        {
            
            // Print a boolean value
            case JSON_VALUE_BOOLEAN:
                written_characters += (size_t) sprintf(&_buffer[written_characters],"%s",p_value->boolean ? "true" : "false");
                break;
            
            // Print an integer value
            case JSON_VALUE_INTEGER:
                written_characters += (size_t) sprintf(&_buffer[written_characters],"%lld", p_value->integer);
                break;

            // Print a floating point value
            case JSON_VALUE_NUMBER:
                written_characters += (size_t) sprintf(&_buffer[written_characters],"%g", p_value->number);
                break;

            // Print a string
            case JSON_VALUE_STRING:
            {

                // Initialized data
                size_t len = strlen(p_value->string);

                // Formatting
                written_characters += (size_t) sprintf(&_buffer[written_characters], "\"");
                
                // Iterate over each character
                for (size_t i = 0; i < len; i++)
                {

                    // Check for an escape sequence
                    switch(p_value->string[i])
                    {

                        // Double quote
                        case '\"':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = '\"';

                            break;

                        // Backslash
                        case '\\':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = '\\';

                            break;

                        // Forward slash
                        case '/':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = '/';

                            break;
                        
                        // Backspace
                        case '\b':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = 'b';

                            break;
                        
                        // Form feed
                        case '\f':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = 'f';

                            break;
                        
                        // Line feed
                        case '\n':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = 'n';

                            break;
                        
                        // Carriage return
                        case '\r':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = 'r';

                            break;
                        
                        // Horizontal tab
                        case '\t':
                            _buffer[written_characters++] = '\\';
                            _buffer[written_characters++] = 't';

                            break;

                        // TODO: Unicode
                        // TODO:case '\u':
                            // TODO:
                            // TODO:break;
                        
                        // Default
                        default:
                            _buffer[written_characters++] = p_value->string[i];
                    }
                }
            
                // Formatting
                written_characters += (size_t) sprintf(&_buffer[written_characters], "\"");
                
                // Done
                break;
            }
            
            // Print an object
            case JSON_VALUE_OBJECT:
            {
                if ( p_value->object )
                {
                    // Initialized data
                    size_t        property_count = dict_values(p_value->object, 0);
                    const char **keys            = 0;
                    json_value **values          = 0;

                    written_characters += (size_t) sprintf(&_buffer[written_characters],"{");

                    if ( property_count == 0 )
                        goto done;

                    keys   = JSON_REALLOC(0, property_count * sizeof(char*));
                    values = JSON_REALLOC(0, property_count * sizeof(json_value*));

                    dict_keys(p_value->object, keys);
                    dict_values(p_value->object, (void **)values);
                    for (size_t i = 0; i < property_count-1; i++)
                    {
                        written_characters += (size_t) sprintf(&_buffer[written_characters],"\"%s\":",keys[i]);
                        json_value_serialize(values[i],_buffer);
                        written_characters += (size_t) sprintf(&_buffer[written_characters],",");
                    }
                    written_characters += (size_t) sprintf(&_buffer[written_characters],"\"%s\":",keys[property_count-1]);
                    json_value_serialize(values[property_count-1], _buffer);

                    if ( JSON_REALLOC(keys, 0) ) goto failed_to_free;
                    if ( JSON_REALLOC(values, 0) ) goto failed_to_free;
                    done:
                    written_characters += (size_t) sprintf(&_buffer[written_characters],"}");
                }
                break;
            }
            
            // Print an array
            case JSON_VALUE_ARRAY:
            {
                
                // Initialized data
                size_t       element_count = 0;
                json_value **elements      = 0;

                // Error check
                if ( p_value->list == (void *) 0 )
                    goto no_list;

                // Get the contents of the array
                {

                    // Get the quantity of elements                    
                    array_get(p_value->list, 0,&element_count);

                    // Allocate memory for the elements
                    elements = JSON_REALLOC(0, element_count * sizeof(json_value*));

                    // Error check
                    if ( elements == (void *) 0 )
                        goto no_mem;

                    // Get the contents of the array
                    array_get(p_value->list, (void **)elements, 0);
                }
                
                // Formatting
                written_characters += (size_t) sprintf(&_buffer[written_characters],"[");

                // Print the first element
                if ( element_count )
                    json_value_serialize(elements[0], _buffer);
                
                // Iterate over each element
                for (size_t i = 1; i < element_count; i++)
                {

                    // Formatting
                    written_characters += (size_t) sprintf(&_buffer[written_characters], ",");

                    // Print the value
                    json_value_serialize(elements[i], _buffer);
                }

                // Free the element
                if ( JSON_REALLOC(elements, 0) ) goto failed_to_free;

                // Formatting
                written_characters += (size_t) sprintf(&_buffer[written_characters], "]");
            }
                break;
            
            default:

                // Error
                return 0;
            }
    }

    // Success
    return 1;

    no_list:
    no_mem:
        return 0;

    // Error handling
    {
        // Standard library errors
        {
            failed_to_free:
                #ifndef NDEBUG
                    printf("[Standard Library] Call to \"realloc\" returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

int json_value_print ( const json_value *const p_value )
{
    
    // Edge case
    if ( p_value == 0 ) goto no_value;

    // Error checking
    if ( p_value->type == JSON_VALUE_INVALID ) goto invalid_value_type;

    // Strategy
    switch (p_value->type)
    {

        // Boolean 
        case JSON_VALUE_BOOLEAN:

            // Write the boolean to standard out
            printf("%s",p_value->boolean ? "true" : "false");

            // Done
            break;
        
        // Integer
        case JSON_VALUE_INTEGER:

            // Write the integer to standard out
            printf("%lld", p_value->integer);

            // Done
            break;

        // Floating point
        case JSON_VALUE_NUMBER:

            // Write the float to standard out
            printf("%g", p_value->number);

            // Done
            break;


        // String
        case JSON_VALUE_STRING:
        {

            // Initialized data
            size_t len = strlen(p_value->string);

            // Formatting
            putchar('\"');
            
            // Iterate over each character
            for ( size_t i = 0; i < len; i++ )
            {

                // Escape sequence
                switch ( p_value->string[i] )
                {

                    // Double quote
                    case '\"':

                        // Write the double quote to standard out
                        printf("\\\"");

                        // Done
                        break;

                    // Backslash
                    case '\\':
                    
                        // Write the backslash to standard out
                        printf("\\\\");

                        // Done
                        break;

                    // Forward slash
                    case '/':
                    
                        // Write the forward slash to standard out
                        printf("\\/");

                        // Done
                        break;
                    
                    // Backspace
                    case '\b':
                    
                        // Write the backspace to standard out
                        printf("\\\b");

                        // Done
                        break;
                    
                    // Form feed
                    case '\f':
                    
                        // Write the form feed to standard out
                        printf("\\\f");

                        // Done
                        break;
                    
                    // Line feed
                    case '\n':
                    
                        // Write the line feed to standard out
                        printf("\\\n");

                        // Done
                        break;
                    
                    // Carriage return
                    case '\r':
                    
                        // Write the carriage return to standard out
                        printf("\\\r");

                        // Done
                        break;
                    
                    // Horizontal tab
                    case '\t':
                    
                        // Write the horizontal tab to standard out
                        printf("\\\t");

                        // Done
                        break;

                    // TODO: Unicode
                    // TODO:case '\u':
                        // TODO:
                        // TODO:break;
                    
                    // Default
                    default:

                        // Print the character to standard out
                        putchar(p_value->string[i]);
                }
            }
        
            // End the string
            putchar('\"');

            // Done
            break;
        }
        
        // Object
        case JSON_VALUE_OBJECT:
        {
            
            // Initialized data
            size_t        property_count = dict_values(p_value->object, 0);
            const char **keys            = 0;
            json_value **values          = 0;

            // Edge case
            if ( property_count == 0 ) goto done;

            // Allocate memory for the keys
            keys = JSON_REALLOC(0, property_count * sizeof(char*));
            
            // Error check
            if ( keys == (void *) 0 ) goto no_mem;
            
            // Dump the keys
            dict_keys(p_value->object, keys);

            // Allocate memory for the values
            values = JSON_REALLOC(0, property_count * sizeof(json_value*));
            
            // Error check
            if ( keys == (void *) 0 ) goto no_mem;

            // Dump the values
            dict_values(p_value->object, (void **)values);

            // Start printing the object
            printf("{");

            // Iterate through each property, save for the final property
            for (size_t i = 0; i < property_count-1; i++)
            {

                // Print the key to standard out
                printf("\"%s\":",keys[i]);

                // Print the value to standard out
                json_value_print(values[i]);

                // Print a comma delimiter to standard out
                printf(",");
            }

            // Print the key of the final property
            printf("\"%s\":",keys[property_count-1]);

            // Print the value of the final property
            json_value_print(values[property_count-1]);

            // Clean up
            if ( JSON_REALLOC(keys, 0) ) goto failed_to_free;
            if ( JSON_REALLOC(values, 0) ) goto failed_to_free;

            done:

            // Finish printing the object
            printf("}");

            // Done
            break;
        }
        
        // Array
        case JSON_VALUE_ARRAY:
        {
            
            // Initialized data
            size_t       element_count = 0;
            json_value **elements      = 0;

            // Error check
            if ( p_value->list == (void *) 0 ) goto no_list;
        
            // Get the quantity of elements in the array                  
            array_get(p_value->list, 0, &element_count);

            // Allocate memory for the elements
            elements = JSON_REALLOC(0, element_count * sizeof(json_value*));

            // Error check
            if ( elements == (void *) 0 ) goto no_mem;

            // Get the contents of the array
            array_get(p_value->list, (void **)elements, 0);
            
            // Start printing the array
            printf("[");

            // Print the first element
            if ( element_count ) json_value_print(elements[0]);
            
            // Iterate over each element
            for (size_t i = 1; i < element_count; i++)
            {

                // Print a comma delimiter to standard out
                printf(",");

                // Print the value to standard out
                json_value_print(elements[i]);
            }

            // Clean up
            if ( JSON_REALLOC(elements, 0) ) goto failed_to_free;

            // Finish printing the array
            printf("]");

            // Done
            break;
        }
        
        // Default
        default:

            // Error handling
            goto unrecognized_json_type;
    }

    // Success
    return 1;
    
    // This branch handles "null"
    no_value:

        // Print null to standard out
        printf("null");

        // Success
        return 1;

    no_list:
    unrecognized_json_type:
    invalid_value_type:
        return 0;

    // Error handling
    {
        
        // Argument check
        {
            no_mem:
                #ifndef NDEBUG
                    log_error("[Standard Library] Failed to allocate memory in call to function \"%s\"\n",__FUNCTION__);
                #endif

                // Error
                return 0;
        }
        
        // Standard library errors
        {
            failed_to_free:
                #ifndef NDEBUG
                    printf("[Standard Library] Call to \"realloc\" returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

int json_value_fprint ( const json_value *const p_value, FILE *p_f )
{
    
    // Null case
    if ( p_value == 0 ) fprintf(p_f,"null");

    // Everything else
    else
    {

        // Print the value
        switch (p_value->type)
        {
            // Print a boolean value
            case JSON_VALUE_BOOLEAN:
                fprintf(p_f,"%s",p_value->boolean ? "true" : "false");
                break;
            
            // Print an integer value
            case JSON_VALUE_INTEGER:
                fprintf(p_f,"%lld", p_value->integer);
                break;

            // Print a floating point value
            case JSON_VALUE_NUMBER:
                fprintf(p_f,"%g", p_value->number);
                break;


            // Print a string
            case JSON_VALUE_STRING:
            {

                // Initialized data
                size_t len = strlen(p_value->string);

                // Formatting
                fprintf(p_f, "\"");
                
                // Iterate over each character
                for (size_t i = 0; i < len; i++)
                {

                    // Check for an escape sequence
                    switch(p_value->string[i])
                    {

                        // Double quote
                        case '\"':
                            putc('\\', p_f);
                            putc('\"', p_f);

                            break;

                        // Backslash
                        case '\\':
                            putc('\\', p_f);
                            putc('\\', p_f);

                            break;

                        // Forward slash
                        case '/':
                            putc('\\', p_f);
                            putc('/', p_f);

                            break;
                        
                        // Backspace
                        case '\b':
                            putc('\\', p_f);
                            putc('b', p_f);

                            break;
                        
                        // Form feed
                        case '\f':
                            putc('\\', p_f);
                            putc('f', p_f);

                            break;
                        
                        // Line feed
                        case '\n':
                            putc('\\', p_f);
                            putc('n', p_f);

                            break;
                        
                        // Carriage return
                        case '\r':
                            putc('\\', p_f);
                            putc('r', p_f);

                            break;
                        
                        // Horizontal tab
                        case '\t':
                            putc('\\', p_f);
                            putc('t', p_f);

                            break;

                        // TODO: Unicode
                        // TODO:case '\u':
                            // TODO:
                            // TODO:break;
                        
                        // Default
                        default:
                            putc(p_value->string[i], p_f);
                    }
                }
            
                // Formatting
                fprintf(p_f, "\"");

                // Done
                break;
            }
            
            // Print an object
            case JSON_VALUE_OBJECT:
            {
                if ( p_value->object )
                {
                    // Initialized data
                    size_t        property_count = dict_values(p_value->object, 0);
                    const char **keys            = 0;
                    json_value **values          = 0;

                    fprintf(p_f,"{");

                    if ( property_count == 0 ) goto done;

                    keys   = JSON_REALLOC(0, property_count * sizeof(char*));
                    values = JSON_REALLOC(0, property_count * sizeof(json_value*));

                    dict_keys(p_value->object, keys);
                    dict_values(p_value->object, (void **)values);
                    for (size_t i = 0; i < property_count-1; i++)
                    {
                        fprintf(p_f,"\"%s\":",keys[i]);
                        json_value_fprint(values[i],p_f);
                        fprintf(p_f,",");
                    }
                    fprintf(p_f,"\"%s\":",keys[property_count-1]);
                    json_value_fprint(values[property_count-1],p_f);

                    if ( JSON_REALLOC(keys, 0) ) goto failed_to_free;
                    if ( JSON_REALLOC(values, 0) ) goto failed_to_free;
                    done:
                    fprintf(p_f,"}");
                    
                    // Done
                    break;
                }
            }
            
            // Print an array
            case JSON_VALUE_ARRAY:
            {
                
                // Initialized data
                size_t       element_count = 0;
                json_value **elements      = 0;

                // Error check
                if ( p_value->list == (void *) 0 ) goto no_list;

                // Get the contents of the array
                {

                    // Get the quantity of elements                    
                    array_get(p_value->list, 0,&element_count);

                    // Allocate memory for the elements
                    elements = JSON_REALLOC(0, element_count * sizeof(json_value*));

                    // Error check
                    if ( elements == (void *) 0 ) goto no_mem;

                    // Get the contents of the array
                    array_get(p_value->list, (void **)elements, 0);
                }
                
                // Formatting
                fprintf(p_f,"[");

                // Print the first element
                if ( element_count ) json_value_fprint(elements[0],p_f);
                
                // Iterate over each element
                for (size_t i = 1; i < element_count; i++)
                {

                    // Formatting
                    fprintf(p_f, ",");

                    // Print the value
                    json_value_fprint(elements[i],p_f);
                }

                // Free the element
                if ( JSON_REALLOC(elements, 0) ) goto failed_to_free;

                // Formatting
                fprintf(p_f, "]");

                // Done
                break;
            }
            
            default:

                // Error
                return 0;
            }
    }

    // Success
    return 1;
    
    no_list:
        return 0;

    // Error handling
    {
        
        // Argument check
        {
            no_mem:
                #ifndef NDEBUG
                    log_error("[Standard Library] Failed to allocate memory in call to function \"%s\"\n",__FUNCTION__);
                #endif

                // Error
                return 0;

            failed_to_free:
                #ifndef NDEBUG
                    printf("[Standard Library] Call to \"realloc\" returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return 0;
        }
    }
}

void json_value_free ( json_value *p_value )
{
    
    // Argument errors
    if ( p_value == (void *)0 ) return;
    
    // Strategy
    switch ( p_value->type )
    {
        case JSON_VALUE_INVALID:
        case JSON_VALUE_BOOLEAN:
        case JSON_VALUE_INTEGER:
        case JSON_VALUE_NUMBER:
        
            // Done
            break;
            
        case JSON_VALUE_STRING:

            // Free the string
            if ( JSON_REALLOC(p_value->string, 0) ) goto failed_to_free;

            // Done
            break;

        case JSON_VALUE_OBJECT:

            // Free each value from the dictionary
            dict_free_clear(p_value->object, (void(*)(const void *const))json_value_free);

            // Destroy the dictionary
            dict_destroy(&p_value->object);

            // Done
            break;

        case JSON_VALUE_ARRAY:

            // Free each value from the array
            array_free_clear(p_value->list, (void(*)(void *))json_value_free);

            // Destroy the array
            array_destroy(&p_value->list);

            // Done
            break;
        
        default:

            // Done
            break;
    }

    // Free the value
    if ( JSON_REALLOC(p_value, 0) ) goto failed_to_free;
    
    // Done
    return;

    // Error handling
    {

        // Standard library errors
        {
            failed_to_free:
                #ifndef NDEBUG
                    printf("[Standard Library] Call to \"realloc\" returned an erroneous value in call to function \"%s\"\n", __FUNCTION__);
                #endif

                // Error
                return;
        }
    }
}

void json_exit ( void )
{
    
    // State check
    if ( initialized == false ) return;

    // Clean up the log library
    log_exit();

    // Clean up the sync library
    sync_exit();

    // Clean up the hash cache library
    hash_cache_exit();

    // Clean up the array library
    array_exit();

    // Clean up the dict library
    dict_exit();

    // Clear the initialized flag
    initialized = false;

    // Done
    return;
}
