#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../cJSON.h"

#define OPLK_PATH_MAX_LEN 512 

//return true - success 
static cJSON_bool set_sr_object(cJSON* item, char* xpath);
static cJSON_bool set_sr_array(cJSON * item, char* xpath);

static cJSON_bool set_sr_value(cJSON* item, char* xpath)
{
   switch ((item->type) & 0xFF)
   {
        case cJSON_NULL:
            printf ("xpath: %s\n", xpath);
            printf ("input null: sr_delete_item(%s)\n", xpath); 
            break;
        case cJSON_False:
            printf ("xpath: %s\n", xpath);
            printf("val false: sr_val.data.bool_val = %s\n",  "false");
            break;
        case cJSON_True:
            printf ("xpath: %s\n", xpath);
            printf("val true: sr_val.data.bool_val = %s\n", "true");
            break;
        case cJSON_Number:
	    /*TODO:need to distinqush: 
	      (u)int8_t;(u)int16_t;(u)int32_t;(u)int64_t;decimal64_t; */
            printf ("xpath: %s\n", xpath);
            printf("val number: sr_val.data.decimal64_val = %f\n", item->valuedouble);
            printf("val number: sr_val.data.uint32_val = %d\n", item->valueint);
            break;
        case cJSON_Raw:
            printf ("xpath: %s\n", xpath);
            printf("val raw: sr_val.data.anydata_val = %s\n", item->valuestring);
            break;
        case cJSON_String:
            printf ("xpath: %s\n", xpath);
            printf("val string: sr_val.data.string_val = %s\n", item->valuestring);
            break;
        case cJSON_Array:
            return set_sr_array(item, xpath);
        case cJSON_Object:
            return set_sr_object(item, xpath);
        default:
            printf ("unknow json data type\n");
            return false;
    }

    return true;
}

static cJSON_bool set_sr_array(cJSON * item, char* xpath)
{
    char acXpath[OPLK_PATH_MAX_LEN];
    cJSON *current_element = item->child;

    //printf ("array: %s\n", cJSON_Print(item));
    // *output_pointer = '['; 

    while (current_element != NULL)
    {
       /*add array key into xpath, assume the key is always firstattribute
        TODO: how to support multi-dimentional array? */
        if (!cJSON_IsObject(current_element)) {
	//leaf list: use xpath as is.
            printf ("leaf-list\n");
            snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s", xpath); 
	}
        else {		   
            printf ("list\n");
	//list with key:  add xpath with [key=value] 
            snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s[%s='%s']", xpath, current_element->child->string, current_element->child->valuestring); 
        }
        printf ("array acXpath: %s\n", acXpath);

        if (!set_sr_value(current_element, acXpath))
        {
           return false;
        }
        current_element = current_element->next;
    }
//    *output_pointer++ = ']'; 

    return true;
}

static cJSON_bool set_sr_object(cJSON * item, char* xpath)
{
    cJSON *current_item = item->child;
    char acXpath[OPLK_PATH_MAX_LEN];

/*    *output_pointer++ = '{'; */

    while (current_item)
    {
        /* print key */

	/*append key to the xpath */
        snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s/%s", xpath, current_item->string);
        /* print value */

	/*set value */
        if (!set_sr_value(current_item, acXpath))
        {
            return false;
        }

        current_item = current_item->next;
    }

    //  *output_pointer++ = '}';

    return true;
}

int CJSON_CDECL main(int argc, char* argv[])
{
    char *json;
    cJSON *cjson;
    size_t length;
    FILE *f;
    char acXpath[OPLK_PATH_MAX_LEN];

    /* print the version */
    //printf("cJSON Version: %s\n", cJSON_Version());

    if (argc < 2) {
        printf("Missing input json file\n");
	printf("Usage: ./oplk_set_sr <file>\n");
	exit (-1);
    }

    f = fopen (argv[1], "rb");

    if (f) {
      fseek (f, 0, SEEK_END);
      length = (size_t)ftell (f);
      fseek (f, 0, SEEK_SET);
      json = (char*) malloc (length);
      if (json) {
        fread (json, 1, length, f);
      }
      fclose (f);
    }
    
    cjson = cJSON_Parse(json);
    //TODO: find ynag schema  
    snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s", "ietf-system:");
    set_sr_value (cjson, acXpath);
  //  printf ("JSON output:\n%s\n", cJSON_Print(cjson));
    return 0;
}
