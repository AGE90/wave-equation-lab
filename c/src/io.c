#include "wave.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

static void config_error(const char* message) {
    printf("Error: Invalid damping configuration: %s\n", message);
    exit(1);
}

static DampingProfileType parse_damping_profile_type(const char* name) {
    if (strcmp(name, "none") == 0) return DAMPING_PROFILE_NONE;
    if (strcmp(name, "x_split") == 0) return DAMPING_PROFILE_X_SPLIT;
    if (strcmp(name, "circular_region") == 0) return DAMPING_PROFILE_CIRCULAR_REGION;
    if (strcmp(name, "absorbing_boundary") == 0) return DAMPING_PROFILE_ABSORBING_BOUNDARY;
    config_error("damping_profile.type must be none, x_split, circular_region, or absorbing_boundary");
    return DAMPING_PROFILE_NONE;
}

Config load_config(const char* filepath) {
    Config cfg = {0};
    cfg.c_alt = -1.0f; // negative means disabled
    // sensible defaults
    cfg.initial_type = INITIAL_ZERO;
    cfg.forcing_type = FORCING_NONE;
    cfg.forcing_f = 30.0f;
    cfg.forcing_f1 = 60.0f;
    cfg.forcing_t1 = 1.0f;
    cfg.damping_profile_type = DAMPING_PROFILE_NONE;
    cfg.damping_power = 2.0f;
    
    FILE* fp = fopen(filepath, "rb");
    if (!fp) {
        printf("Error: Could not open config file %s\n", filepath);
        exit(1);
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char* content = (char*)malloc(length + 1);
    if (content) {
        if(fread(content, 1, length, fp)) {}; // Ignore return to suppress warnings
        content[length] = '\0';
    }
    fclose(fp);
    
    if (!content) {
        printf("Error: Memory allocation failed\n");
        exit(1);
    }
    
    cJSON* json_root = cJSON_Parse(content);
    if (!json_root) {
        printf("Error: Failed to parse JSON config in %s\n", filepath);
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error before: %s\n", error_ptr);
        }
        free(content);
        exit(1);
    }
    
    cJSON* item;
    // helper lambdas (as static functions) are not available in C89; use inline parsing here
    if ((item = cJSON_GetObjectItem(json_root, "nx"))) cfg.nx = item->valueint;
    if ((item = cJSON_GetObjectItem(json_root, "ny"))) cfg.ny = item->valueint;
    if ((item = cJSON_GetObjectItem(json_root, "nt"))) cfg.nt = item->valueint;
    if ((item = cJSON_GetObjectItem(json_root, "dt"))) cfg.dt = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "dx"))) cfg.dx = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "dy"))) cfg.dy = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "c"))) cfg.c_speed = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "c_alt"))) cfg.c_alt = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "x_limit"))) cfg.x_interface = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "media_type"))) cfg.media_type = item->valueint;
    else if (cfg.c_alt > 0.0f) cfg.media_type = 1; // Backward compatibility
    if ((item = cJSON_GetObjectItem(json_root, "lens_radius"))) cfg.lens_radius = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "lens_x"))) cfg.lens_x = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "lens_y"))) cfg.lens_y = item->valuedouble;
    if ((item = cJSON_GetObjectItem(json_root, "damping"))) cfg.damping = item->valuedouble;
    cfg.damping_alt = cfg.damping;
    cfg.damping_edge = cfg.damping;

    cJSON* damping_obj = cJSON_GetObjectItem(json_root, "damping_profile");
    if (damping_obj && damping_obj->type == cJSON_Object) {
        cJSON* profile_item = cJSON_GetObjectItem(damping_obj, "type");
        if (!profile_item || profile_item->type != cJSON_String || !profile_item->valuestring) {
            config_error("damping_profile.type is required and must be a string");
        }
        cfg.damping_profile_type = parse_damping_profile_type(profile_item->valuestring);
        if ((item = cJSON_GetObjectItem(damping_obj, "damping_alt"))) cfg.damping_alt = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "x_limit"))) cfg.damping_x_limit = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "radius"))) cfg.damping_radius = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "x"))) cfg.damping_x = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "y"))) cfg.damping_y = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "width"))) cfg.damping_width = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "edge_damping"))) cfg.damping_edge = item->valuedouble;
        if ((item = cJSON_GetObjectItem(damping_obj, "power"))) cfg.damping_power = item->valuedouble;
    }

    if (cfg.damping < 0.0f || cfg.damping_alt < 0.0f || cfg.damping_edge < 0.0f || cfg.damping_power < 0.0f) {
        config_error("damping values and power must be non-negative");
    }
    if (cfg.damping_profile_type == DAMPING_PROFILE_ABSORBING_BOUNDARY && cfg.damping_width <= 0.0f) {
        config_error("absorbing_boundary requires width > 0");
    }
    if (cfg.damping_profile_type == DAMPING_PROFILE_CIRCULAR_REGION && cfg.damping_radius <= 0.0f) {
        config_error("circular_region requires radius > 0");
    }
    // Parse initial_type (string or int)
    if ((item = cJSON_GetObjectItem(json_root, "initial_type"))) {
        if (item->type == cJSON_String && item->valuestring) {
            cfg.initial_type = parse_initial_type(item->valuestring, cfg.initial_type);
        } else {
            cfg.initial_type = (InitialType)item->valueint;
        }
    }

    // Parse forcing: accept either a top-level forcing_type or a nested object 'forcing'
    if ((item = cJSON_GetObjectItem(json_root, "forcing_type"))) {
        cfg.forcing_type = (ForcingType)item->valueint;
    }
    cJSON* forcing_obj = cJSON_GetObjectItem(json_root, "forcing");
    if (forcing_obj && forcing_obj->type == cJSON_Object) {
        cJSON* ftype = cJSON_GetObjectItem(forcing_obj, "type");
        if (ftype) {
            if (ftype->type == cJSON_String && ftype->valuestring) {
                cfg.forcing_type = parse_forcing_type(ftype->valuestring, cfg.forcing_type);
            } else {
                cfg.forcing_type = (ForcingType)ftype->valueint;
            }
        }
        cJSON* fval;
        if ((fval = cJSON_GetObjectItem(forcing_obj, "f"))) cfg.forcing_f = fval->valuedouble;
        if ((fval = cJSON_GetObjectItem(forcing_obj, "f1"))) cfg.forcing_f1 = fval->valuedouble;
        if ((fval = cJSON_GetObjectItem(forcing_obj, "t1"))) cfg.forcing_t1 = fval->valuedouble;
    }
    if ((item = cJSON_GetObjectItem(json_root, "output_freq"))) cfg.output_freq = item->valueint;
    
    if (cfg.forcing_type == FORCING_NONE) {
        // default: no forcing; keep the initial_type as set (default gaussian)
    }

    cfg.initial_fn = resolve_initial_fn(cfg.initial_type);
    cfg.forcing_fn = resolve_forcing_fn(cfg.forcing_type);

    cJSON_Delete(json_root);
    free(content);
    return cfg;
}

void write_frame(FILE* bin_fp, float* u, int nx, int ny) {
    size_t written = fwrite(u, sizeof(float), nx * ny, bin_fp);
    if (written != (size_t)(nx * ny)) {
        printf("Error writing to binary file!\n");
    }
}
