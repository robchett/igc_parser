#pragma once

#include <mxml.h>

mxml_node_t *new_text_node(mxml_node_t *parent, char *tag, char *value);
mxml_node_t *new_cdata_node(mxml_node_t *parent, char *tag, char *value);