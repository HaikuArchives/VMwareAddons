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
	static ino_t	current_inode;

public:
					VMWNode(const char* _name, VMWNode* _parent);
	virtual			~VMWNode();

	ssize_t			CopyPathTo(char* buffer, size_t buffer_length, const char* to_append = NULL);
	void			DeleteChildIfExists(const char* name);
	VMWNode*		GetChild(const char* name);
	VMWNode*		GetChild(ino_t inode);
	ino_t			GetInode() const { return inode; }
	const char*		GetName() const { return name; }

private:
	char*			name;
	size_t			name_length;
	VMWNode* 		parent;
	ino_t			inode;
	vmw_list_item*	children;
};

#endif
