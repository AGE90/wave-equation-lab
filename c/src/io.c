#include "wave.h"
#include <string.h>
#include <stdlib.h>
#include "cJSON.h"

Config load_config(const char* filepath) {
    Config cfg = {0};
    cfg.c_alt = -1.0f; // negative means disabled
    
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
    if ((item = cJSON_GetObjectItem(json_root, "source_type"))) cfg.source_type = item->valueint;
    if ((item = cJSON_GetObjectItem(json_root, "output_freq"))) cfg.output_freq = item->valueint;
    
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
