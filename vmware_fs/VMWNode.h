#ifndef VMW_NODE_H
#define VMW_NODE_H


#include <List.h>
#include <String.h>


class VMWNode;


typedef struct vmw_list_item {
	VMWNode* node;
	vmw_list_item* next;
} vmw_list_item;


class VMWNode {
public:
					VMWNode(const char* name, VMWNode* parent);
	virtual			~VMWNode();

	ssize_t			CopyPathTo(char* buffer, size_t bufferLength, const char* toAppend = NULL);
	void			DeleteChildIfExists(const char* name);
	VMWNode*		GetChild(const char* name);
	VMWNode*		GetChild(ino_t inode);
	ino_t			GetInode() const { return fInode; }
	const char*		GetName() const { return fName; }
	VMWNode*		GetParent() const { return fParent; }

private:
	static ino_t	sCurrentInode;

	vmw_list_item*	fChildren;
	ino_t			fInode;
	char*			fName;
	size_t			fNameLength;
	VMWNode* 		fParent;
};

#endif
