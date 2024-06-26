#pragma once

/* kmod.c */
int kmod_find(const char *name_prefix, int min_version, int max_version);
int kmod_find_any_version(const char *name_prefix);