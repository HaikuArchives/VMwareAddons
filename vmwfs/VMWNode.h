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

	void			DeleteChildIfExists(const char* name);
	VMWNode*		GetChild(const char* name);
	VMWNode*		GetChild(ino_t inode);
	char*			GetChildPath(const char* name);
	ino_t			GetInode() const { return inode; }
	const char*		GetName() const { return name; }
	char*			GetPath(size_t length = 0);

private:
	char*			name;
	VMWNode* 		parent;
	ino_t			inode;
	vmw_list_item*	children;
};

#endif
