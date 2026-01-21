#include "VMWNode.h"

#include <KernelExport.h>
#include <stdio.h>
#include <stdlib.h>


ino_t VMWNode::sCurrentInode = 2;


VMWNode::VMWNode(const char* name, VMWNode* parent)
	:
	fChildren(NULL),
	fParent(parent)
{
	fName = strdup(name);
	fNameLength = strlen(fName);
	fInode = sCurrentInode;
	sCurrentInode++;
}


VMWNode::~VMWNode()
{
	vmw_list_item* item = fChildren;
	while (item != NULL) {
		delete item->node;

		vmw_list_item* nextItem = item->next;
		free(item);
		item = nextItem;
	}
	free(fName);
}


ssize_t
VMWNode::CopyPathTo(char* buffer, size_t bufferLength, const char* toAppend)
{
	if (fParent == NULL) { // This is the root node
		if (toAppend == NULL || toAppend[0] == '\0' || strcmp(toAppend, ".") == 0
			|| strcmp(toAppend, "..") == 0) {
			memset(buffer, 0, bufferLength);
			return 0;
		}

		size_t length = strlen(toAppend);

		if (length >= bufferLength)
			return -1;

		strcpy(buffer, toAppend);
		return length;
	}

	if (toAppend != NULL) {
		if (strcmp(toAppend, "..") == 0)
			return fParent->CopyPathTo(buffer, bufferLength);

		if (strcmp(toAppend, ".") == 0)
			toAppend = NULL;
	}

	ssize_t previousLength = fParent->CopyPathTo(buffer, bufferLength);

	if (previousLength < 0)
		return -1;

	bool appendSlash = (previousLength > 0);

	size_t newLength = previousLength + (appendSlash ? 1 : 0) + fNameLength;

	if (toAppend != NULL)
		newLength += strlen(toAppend) + 1;

	if (newLength >= bufferLength) // No space left
		return -1;

	if (appendSlash) // Need to add a slash after the previous item
		strcat(buffer, "/");

	strcat(buffer, fName);

	if (toAppend != NULL) {
		strcat(buffer, "/");
		strcat(buffer, toAppend);
	}

	return newLength;
}


void
VMWNode::DeleteChildIfExists(const char* name)
{
	vmw_list_item* item = fChildren;
	vmw_list_item* prevItem = NULL;

	while (item != NULL) {
		if (strcmp(item->node->fName, name) == 0) {
			delete item->node;

			if (prevItem == NULL)
				fChildren = item->next;
			else
				prevItem->next = item->next;

			free(item);
			return;
		}
		prevItem = item;
		item = item->next;
	}
}


VMWNode*
VMWNode::GetChild(const char* name)
{
	if (strcmp(name, "") == 0 || strcmp(name, ".") == 0)
		return this;

	if (strcmp(name, "..") == 0)
		return fParent == NULL ? this : fParent;

	vmw_list_item* item = fChildren;

	while (item != NULL) {
		if (strcmp(item->node->fName, name) == 0)
			return item->node;

		if (item->next == NULL)
			break;

		item = item->next;
	}

	vmw_list_item* newItem = (vmw_list_item*)malloc(sizeof(vmw_list_item));

	if (newItem == NULL)
		return NULL;

	newItem->next = NULL;
	newItem->node = new VMWNode(name, this);
	if (newItem->node == NULL) {
		free(newItem);
		return NULL;
	}

	if (item == NULL)
		fChildren = newItem;
	else
		item->next = newItem;

	return newItem->node;
}


VMWNode*
VMWNode::GetChild(ino_t inode)
{
	if (this->fInode == inode)
		return this;

	vmw_list_item* item = fChildren;

	while (item != NULL) {
		VMWNode* node = item->node->GetChild(inode);
		if (node != NULL)
			return node;

		item = item->next;
	}

	return NULL;
}
