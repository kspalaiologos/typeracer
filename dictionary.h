
#ifndef _DICTIONARY_H
#define _DICTIONARY_H

static bool directory_exists(char * path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static int lexicographical_compare(const void * a, const void * b) {
    for(char * astr = *(char **)a, * bstr = *(char **)b; *astr && *bstr; astr++, bstr++)
        if(*astr != *bstr)
            return *astr - *bstr;
    return 0;
}

static vector(char *) list_dictionaries(char * dictionary_dir) {
    vector(char *) list = NULL;
    DIR * dir = opendir(dictionary_dir);
    if(dir == NULL) {
        perror("opendir");
        return list;
    }
    struct dirent * entry;
    while((entry = readdir(dir)) != NULL) {
        if(entry->d_name[0] == '.')
            continue;
        if(strlen(entry->d_name) > 4 && strcmp(entry->d_name + strlen(entry->d_name) - 4, ".dic") == 0)
            vector_push_back(list, strdup(entry->d_name));
    }
    closedir(dir);
    qsort(list, vector_size(list), sizeof(char *), lexicographical_compare);
    return list;
}

static vector(char *) find_dictionaries(void) {
    vector(char *) dictionaries = NULL;
    char * dict_path = NULL;

    if(getenv("TR_DIC") == NULL) {
        if(directory_exists("/usr/share/typeracer-dict"))
            dictionaries = list_dictionaries("/usr/share/typeracer-dict"), dict_path = "/usr/share/typeracer-dict";
        else if(directory_exists("typeracer-dict"))
            dictionaries = list_dictionaries("typeracer-dict"), dict_path = "typeracer-dict";
        else {
            printf("Could not find typeracer dictionaries (/usr/share/typeracer-dict or ./typeracer-dict).\n");
            exit(1);
        }
    } else {
        if(directory_exists(getenv("TR_DIC")))
            dictionaries = list_dictionaries(getenv("TR_DIC")), dict_path = getenv("TR_DIC");
        else {
            printf("Could not find typeracer dictionaries (TR_DIC: %s).\n", getenv("TR_DIC"));
            exit(1);
        }
    }

    if(dictionaries == NULL) {
        printf("Could not find any typeracer dictionaries.\n");
        exit(1);
    }

    chdir(dict_path);

    return dictionaries;
}

#endif
