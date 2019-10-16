/**
 * Copyright 2019 Sergey Tyultyaev
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "macro.h"

char *append_dir_alloc(const char *dir, const char *path)
{
    char *result = NULL;

    CHECK_ERROR(dir != NULL, NULL, "dir == NULL");
    CHECK_ERROR(path != NULL, NULL, "path != NULL");

    size_t result_size = strlen(dir) + strlen(path) + strlen("/") + 1;
    result = malloc(result_size);

    CHECK_ERROR(result != NULL, NULL, "malloc() failed");

    strcpy_s(result, result_size, dir);
    if (strlen(dir) > 0 && dir[strlen(dir) - 1] == '/') {
        strcat_s(result, result_size, "/");
    }
    strcat_s(result, result_size, path);

done:
    return result;
}
