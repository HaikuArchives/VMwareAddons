#ifndef VMW_NODE_H
#define VMW_NODE_H

#include <List.h>
#include <String.h>

#define CACHE_SIZE 500

class VMWNode;

typedef struct vmw_list_item {
	VMWNode* node;
	vmw_list_item* next;

} vmw_list_item;

class VMWNode {
	static ino_t	current_inode;
	static VMWNode*	nodes_with_cached_paths[CACHE_SIZE];
	static uint32	cache_current;

public:
					VMWNode(const char* _name, VMWNode* _parent);
	virtual			~VMWNode();

	void			DeleteChildIfExists(const char* name);
	VMWNode*		GetChild(const char* name);
	VMWNode*		GetChild(ino_t inode);
	char*			GetChildPath(const char* name);
	ino_t			GetInode() const { return inode; }
	const char*		GetName() const { return name; }
	const char*		GetPath(size_t* path_length = NULL);

private:
	char*			_GetPath(size_t* length);
	
	char*			cached_path;
	size_t			cached_path_length;
	int32			cached_path_position;
	char*			name;
	VMWNode* 		parent;
	ino_t			inode;
	vmw_list_item*	children;
};

#endif
