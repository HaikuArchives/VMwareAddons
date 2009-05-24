#include "VMWNode.h"

#include <stdio.h>
#include <stdlib.h>
#include <KernelExport.h>

ino_t VMWNode::current_inode = 1;
VMWNode* VMWNode::nodes_with_cached_paths[CACHE_SIZE];
uint32 VMWNode::cache_current = 0;

VMWNode::VMWNode(const char* _name, VMWNode* _parent)
	: parent(_parent)
{
	name = strdup(_name);
	inode = current_inode;
	current_inode++;
	children = NULL;
	cached_path = NULL;
	cached_path_position = -1;
}

VMWNode::~VMWNode()
{
	vmw_list_item* item = children;
	while (item != NULL) {
		delete item->node;

		vmw_list_item* next_item = item->next;
		free(item);
		item = next_item;
	}
	free(name);

	if (cached_path_position >= 0) {
		free(cached_path);
		nodes_with_cached_paths[cached_path_position] = NULL;
	}
}

void
VMWNode::DeleteChildIfExists(const char* name)
{
	vmw_list_item* item = children;
	vmw_list_item* prev_item = NULL;

	while (item != NULL) {
		if (strcmp(item->node->name, name) == 0) {
			delete item->node;

			if (prev_item == NULL)
				children = item->next;
			else
				prev_item->next = item->next;

			free(item);
			return;

		}
		prev_item = item;
		item = item->next;
	}
}

VMWNode*
VMWNode::GetChild(const char* name)
{
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0)
		return this;

	if (strcmp(name, "..") == 0)
		return (parent == NULL ? this : parent);

	vmw_list_item* item = children;

	while (item != NULL) {
		if (strcmp(item->node->name, name) == 0)
			return item->node;

		if (item->next == NULL)
			break;

		item = item->next;
	}

	vmw_list_item* new_item = (vmw_list_item*)malloc(sizeof(vmw_list_item));

	if (new_item == NULL)
		return NULL;

	new_item->next = NULL;
	new_item->node = new VMWNode(name, this);
	if (new_item->node == NULL) {
		free(new_item);
		return NULL;
	}

	if (item == NULL)
		children = new_item;
	else
		item->next = new_item;

	return new_item->node;
}

VMWNode*
VMWNode::GetChild(ino_t _inode)
{
	if (this->inode == _inode)
		return this;

	vmw_list_item* item = children;

	while (item != NULL) {
		VMWNode* node = item->node->GetChild(_inode);
		if (node != NULL)
			return node;

		item = item->next;
	}

	return NULL;
}

char*
VMWNode::GetChildPath(const char* name)
{
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0)
		return strdup(GetPath());

	if (strcmp(name, "..") == 0)
		return (parent == NULL ? strdup("") : strdup(parent->GetPath()));
	
	if (parent == NULL)
		return strdup(name);
	
	size_t length;
	const char* parent_path = GetPath(&length);
	if (parent_path == NULL)
		return NULL;
	
	char* child_path = (char*)malloc(length + strlen(name) + 1);
	
	strcpy(child_path, parent_path);
	strcat(child_path, "/");
	strcat(child_path, name);
	
	return child_path;
}

const char*
VMWNode::GetPath(size_t* path_length)
{	
	if (cached_path_position < 0) {
		cached_path_length = 0;
		cached_path = _GetPath(&cached_path_length);		
		if (cached_path == NULL)
			return NULL;
	
		if (nodes_with_cached_paths[cache_current] != NULL) {
			free(nodes_with_cached_paths[cache_current]->cached_path);
			nodes_with_cached_paths[cache_current]->cached_path = NULL;
		}
		
		nodes_with_cached_paths[cache_current] = this;
		
		cached_path_position = cache_current;
		
		cache_current++;
		
		if (cache_current >= CACHE_SIZE)
			cache_current = 0;
	}
	
	if (path_length != NULL)
		*path_length = cached_path_length;
	return cached_path;
}

char*
VMWNode::_GetPath(size_t* length)
{
	char* path;

	if (parent == NULL) {
		if (*length == 0)
			*length = 1;
		path = (char*)malloc(*length);
		if (path == NULL)
			return NULL;
		path[0] = path[*length] = '\0';
		return path;
	}

	*length += strlen(name) + 1;
	path = parent->_GetPath(length);

	if (path == NULL)
			return NULL;

	if (path[0] != '\0')
		strcat(path, "/");

	strcat(path, name);

	return path;
}
