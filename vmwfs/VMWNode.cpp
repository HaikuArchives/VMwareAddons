#include "VMWNode.h"

#include <stdio.h>
#include <stdlib.h>
#include <KernelExport.h>

ino_t VMWNode::current_inode = 2;

VMWNode::VMWNode(const char* _name, VMWNode* _parent)
	: parent(_parent)
{
	name = strdup(_name);
	inode = current_inode;
	current_inode++;
	children = NULL;
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
		return this->GetPath();

	if (strcmp(name, "..") == 0)
		return (parent == NULL ? this->GetPath() : parent->GetPath());

	char* child_path = GetPath(strlen(name) + 1);
	if (child_path == NULL)
		return NULL;

	if (child_path[0] != '\0')
		strcat(child_path, "/");
	strcat(child_path, name);

	return child_path;
}

char*
VMWNode::GetPath(size_t length)
{
	char* path;

	if (parent == NULL) {
		path = (char*)malloc(length == 0 ? 1 : length);
		path[0] = path[length] = '\0';
		return path;
	}

	path = parent->GetPath(length + strlen(name) + 1);

	if (path[0] != '\0')
		strcat(path, "/");

	strcat(path, name);

	return path;
}
