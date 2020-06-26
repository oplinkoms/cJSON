#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "../cJSON.h"
#include "common.h"

#define OPLK_PATH_MAX_LEN 512 
#define JSON_MODULE_PATH_FILE "../../../openapi/mapping/module_path.json"
#define JSON_NUMBER_TYPE_FILE "../../../openapi/mapping/number_type.json"
static cJSON *module_path = NULL;
static cJSON *number_type = NULL;

//return true - success 
static cJSON_bool set_sr_object(cJSON* item, char* xpath);
static cJSON_bool set_sr_array(cJSON * item, char* xpath);

static cJSON_bool set_sr_number(char *xpath, cJSON *item)
{
    char *data_type;
    cJSON *current_item;
    cJSON *leaf;

    current_item = number_type->child;
    while (current_item != NULL)
    {
        data_type = current_item->string;
        //printf ("data_type: %s\n", data_type);

        //go through array find the leaf node 
        leaf = current_item->child;
        while (leaf != NULL) {
          //url path contains the module path
            if (strstr(xpath, leaf->valuestring)) {
               //printf ("leaf: %s data_type: %s\n", cJSON_Print(leaf), data_type);
                goto set_val;
            }
            leaf = leaf->next;
        }

        current_item = current_item->next;
    }
    return false;

set_val:
    //TODO: timeout leaf has multiple types, need special handling
    printf ("xpath: %s\n", xpath);
    if (strcmp(data_type, "decimal64") == 0)  {
        printf("val number: sr_val.data.decimal64_val = %f\n", item->valuedouble);
    } else if (strcmp(data_type, "uint64") == 0)  {
        printf("val number: sr_val.data.uint64_val = %d\n", item->valueint);
    } 
    else if (strcmp(data_type, "uint32") == 0) { 
        printf("val number: sr_val.data.uint32_val = %d\n", item->valueint);
    }
    else if (strcmp(data_type, "uint16") == 0) {
        printf("val number: sr_val.data.uint16_val = %d\n", item->valueint);
    }
    else if (strcmp(data_type, "uint8") == 0) {
        printf("val number: sr_val.data.uint8_val = %d\n", item->valueint);
    }
    else if (strcmp(data_type, "int64") == 0) {
        printf("val number: sr_val.data.int64_val = %d\n", item->valueint);
    }
    else if (strcmp(data_type, "int32") == 0) {
        printf("val number: sr_val.data.int32_val = %d\n", item->valueint);
    }
    else if (strcmp(data_type, "int16") == 0) {
        printf("val number: sr_val.data.int16_val = %d\n", item->valueint);
    }
    else if (strcmp(data_type, "int8") == 0) {
        printf("val number: sr_val.data.int8_val = %d\n", item->valueint);
    }
	
    return true;
}

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
	    /*need to distinqush: 
	      (u)int8_t;(u)int16_t;(u)int32_t;(u)int64_t;decimal64_t; */
	    set_sr_number(xpath, item);
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

    while (current_element != NULL)
    {
       /*add array key into xpath, assume the key is always firstattribute
        TODO: how to support multi-dimentional array? */
        if (!cJSON_IsObject(current_element)) {
          //leaf list: no key, use xpath as is.
            snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s", xpath); 
	}
        else {		   
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

    return true;
}

static cJSON_bool set_sr_object(cJSON * item, char* xpath)
{
    cJSON *current_item = item->child;
    char acXpath[OPLK_PATH_MAX_LEN];

    while (current_item != NULL)
    {
	/*append key to the xpath */
        snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s/%s", xpath, current_item->string);
	/*set value */
        if (!set_sr_value(current_item, acXpath))
        {
            return false;
        }

        current_item = current_item->next;
    }

    return true;
}

static cJSON_bool rest_json_init(void)
{
    char *json;

    json = read_file(JSON_MODULE_PATH_FILE);
    module_path = cJSON_Parse(json);
    //cJSON_Print(module_path);
    free(json);

    json = read_file(JSON_NUMBER_TYPE_FILE);
    number_type = cJSON_Parse(json);

    free(json);
    //cJSON_Print(number_type);
    return true;
}

static cJSON_bool get_yang_module(char *url_path, char* module_name)
{
    char *module;
    cJSON *current_item;
    cJSON *path;
 
    current_item = module_path->child;
    while (current_item != NULL)
    { 
        module = current_item->string;
	//printf ("module: %s\n", module);

	//go through array find the url path
	path = current_item->child;
	while (path != NULL) {
          //url path contains the module path
       	//printf ("path: %s\n", cJSON_Print(path));
	    if (strstr(url_path, path->valuestring)) {
       	printf ("pathi: %s module: %s\n", cJSON_Print(path), module);
                snprintf(module_name, OPLK_PATH_MAX_LEN, "%s", module);
	        return true;
	    }
	    path = path->next;
	}
	
        current_item = current_item->next;
    }
    return false;
}


int CJSON_CDECL main(int argc, char* argv[])
{
    char *json;
    cJSON *cjson;
    char acXpath[OPLK_PATH_MAX_LEN];
    char acYangModule[OPLK_PATH_MAX_LEN];

    if (argc < 3) {
	printf("Usage: ./oplk_set_sr <file> <url>\n");
	exit (-1);
    }

    rest_json_init();

    json = read_file(argv[1]);
    cjson = cJSON_Parse(json);

    //printf ("JSON data:\n%s\n", cJSON_Print(cjson));
    //find yang schema from url path 
    if (!get_yang_module (argv[2], acYangModule)) {
        printf ("can not find yang module \n");
	return  (-1);
    }

    snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s:", acYangModule);
    //snprintf(acXpath, OPLK_PATH_MAX_LEN, "%s", "ietf-system:");
    set_sr_value (cjson, acXpath);

    free (json);
    return 0;
}
