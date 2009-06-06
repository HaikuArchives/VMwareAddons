#include "VMWNode.h"

#include <stdio.h>
#include <stdlib.h>
#include <KernelExport.h>

ino_t VMWNode::current_inode = 2;

VMWNode::VMWNode(const char* _name, VMWNode* _parent)
	: parent(_parent)
{
	name = strdup(_name);
	name_length = strlen(name);
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

ssize_t
VMWNode::CopyPathTo(char* buffer, size_t buffer_length, const char* to_append)
{
	if (parent == NULL) { // This is the root node
		if (to_append == NULL || to_append[0] == '\0'
			|| strcmp(to_append, ".") == 0 || strcmp(to_append, "..") == 0) {
			memset(buffer, 0, buffer_length);
			return 0;
		}

		size_t length = strlen(to_append);

		if (length >= buffer_length)
			return -1;

		strcpy(buffer, to_append);
		return length;
	}

	if (to_append != NULL) {
		if (strcmp(to_append, "..") == 0)
			return parent->CopyPathTo(buffer, buffer_length);

		if (strcmp(to_append, ".") == 0)
			to_append = NULL;
	}

	ssize_t previous_length = parent->CopyPathTo(buffer, buffer_length);

	if (previous_length < 0)
		return -1;

	bool append_slash = (previous_length > 0);

	size_t new_length = previous_length + (append_slash ? 1 : 0) + name_length;

	if (to_append != NULL)
		new_length += strlen(to_append) + 1;

	if (new_length >= buffer_length) // No space left
		return -1;

	if (append_slash) // Need to add a slash after the previous item
		strcat(buffer, "/");

	strcat(buffer, name);

	if (to_append != NULL) {
		strcat(buffer, "/");
		strcat(buffer, to_append);
	}

	return new_length;
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

