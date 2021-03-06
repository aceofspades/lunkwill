#include "datastructures.h"

/*
 * A simple linked List
 *
 * The list_* functions retrun a pointer to a valid list element
 * The old list/list_elem pointer may be invalid after call
 * */

list_elem *list_add_elem(list_elem *list, void *data)
{
	list_elem *old, *new;
	if(list!=NULL)
	{
		old=list->next;
		new=list->next=old->prev=calloc(1, sizeof(list_elem));
		new->next=old;
		new->prev=list;
		new->data=data;
	}
	else
	{
		new=calloc(1, sizeof(list_elem));
		new->next=new;
		new->prev=new;
		new->data=data;
	}
	return new;
}

list_elem *list_map(list_elem *list, int(func)(void*, va_list), ...)
{
	list_elem *le=list;

	if(le==NULL)return NULL;

	va_list vl, vl_call;
	va_start(vl, func);

	do
	{
RELOOP:
		va_copy(vl_call,vl);
		if(func(le->data, vl_call)!=0)
		{
			nfree(le->data);
			if(le->prev==le)
			{
				nfree(le);
				va_end(vl_call);
				va_end(vl);
				return NULL;
			}
			else
			{
				list_elem *old=le->prev;
				int reloop=0;
				old->next=le->next;
				old->next->prev=old;
				if(le==list)
				{
					list=old;
					reloop=1;
				}
				nfree(le);
				le=old;
				if(reloop) goto RELOOP;
			}
		}
		le=le->next;
		va_end(vl_call);
	}
	while(le!=list);

	va_end(vl);
	return le;
}

void list_clear(list_elem *list)
{
	list_elem *le=list;

	while(le!=NULL)
	{
		nfree(le->data);
		if(le->prev==le)
		{
			nfree(le);
		}
		else
		{
			list_elem *old=le->prev;
			old->next=le->next;
			old->next->prev=old;
			nfree(le);
			le=old;
		}
	}
}


/*
 * A simple unbalanced binary tree
 * */

tree_node *tree_add_elem(tree_node *root_node, void *data, int(cmp_func)(void*,void*,void*), void *cmp_data)
{
	tree_node *old_node_ptr=root_node;
	tree_node *node_ptr=root_node;
	int i;

	while(node_ptr!=NULL)
	{
		if((i=cmp_func(data,node_ptr->list->data,cmp_data))>0)
		{
			old_node_ptr=node_ptr;
			node_ptr=node_ptr->left;
		}
		else if(i<0)
		{
			old_node_ptr=node_ptr;
			node_ptr=node_ptr->right;
		}
		else
		{
			node_ptr->list=list_add_elem(node_ptr->list, data);
			return node_ptr;
		}
	}

	node_ptr=calloc(1,sizeof(tree_node));
	node_ptr->list=list_add_elem(NULL, data);

	if(root_node!=NULL)
	{
		if(i>0)
		{
			old_node_ptr->left=node_ptr;
		}
		else
		{
			old_node_ptr->right=node_ptr;
		}
	}

	return node_ptr;
}

list_elem *tree_get_elem(tree_node *root_node, void *data, int(cmp_func)(void*,void*,void*), void *cmp_data)
{
	tree_node *node_ptr=root_node;
	int i;

	while(node_ptr!=NULL)
	{
		if((i=cmp_func(data,node_ptr->list->data,cmp_data))>0)
		{
			node_ptr=node_ptr->left;
		}
		else if(i<0)
		{
			node_ptr=node_ptr->right;
		}
		else
		{
			node_ptr->list=list_add_elem(node_ptr->list, data);
			return node_ptr->list;
		}
	}

	return NULL;
}

tree_node *tree_rem_elem(tree_node *root_node, void *data, int(cmp_func)(void*,void*,void*), void *cmp_data)
{
	tree_node *root=root_node;
	tree_node *node_ptr=root;
	tree_node *old_node_ptr=NULL;
	int i;

	while(node_ptr!=NULL)
	{
		if((i=cmp_func(data,node_ptr->list->data,cmp_data))>0)
		{
			old_node_ptr=node_ptr;
			node_ptr=node_ptr->left;
		}
		else if(i<0)
		{
			old_node_ptr=node_ptr;
			node_ptr=node_ptr->right;
		}
		else
		{

			list_clear(node_ptr->list);

			if(old_node_ptr==NULL)
			{
				if(node_ptr->left!=NULL)
				{
					root=node_ptr->left;
					node_ptr->left=NULL;
				}
				else
				{
					root=node_ptr->right;
					node_ptr->right=NULL;
					nfree(node_ptr);
					return root;
				}
			}
			else if(old_node_ptr->left==node_ptr)
			{
				if(node_ptr->left!=NULL)
				{
					old_node_ptr->left=node_ptr->left;
					node_ptr->left=NULL;
				}
				else
				{
					old_node_ptr->left=node_ptr->right;
					node_ptr->right=NULL;
					nfree(node_ptr);
					return root;
				}
			}
			else
			{
				if(node_ptr->left!=NULL)
				{
					old_node_ptr->right=node_ptr->left;
					node_ptr->left=NULL;
				}
				else
				{
					old_node_ptr->right=node_ptr->right;
					node_ptr->right=NULL;
					nfree(node_ptr);
					return root;
				}
			}

			tree_node *missing_node=node_ptr->right;
			nfree(node_ptr);

			if(missing_node==NULL) return root;

			node_ptr=root;
			while(node_ptr!=NULL)
			{
				if((i=cmp_func(missing_node->list->data,node_ptr->list->data,cmp_data))>0)
				{
					old_node_ptr=node_ptr;
					node_ptr=node_ptr->left;
				}
				else if(i<0)
				{
					old_node_ptr=node_ptr;
					node_ptr=node_ptr->right;
				}
				else
				{
//					log_write("There is something wrong with the tree",LOG_FATAL);
					return NULL;
				}
			}
			if(i>0)
			{
				old_node_ptr->left=missing_node;
			}
			else
			{
				old_node_ptr->right=missing_node;
			}
			return root;
		}
	}
	return root;
}


void tree_clear(tree_node *active_node)
{
	if(active_node==NULL) return;
	tree_clear(active_node->left);
	tree_clear(active_node->right);
	list_clear(active_node->list);
	nfree(active_node);
}


/*
 * A simple radix sort implementation
 * */

void *rsort(void *data, int num_elem, int size_elem, int get_bit(void*, int))
{
	int ct_high,ct_low,high,low,bit=0,i=0;
	char *buf[2];
	char *dat=data;

	buf[0]=calloc(num_elem,size_elem);
	buf[1]=calloc(num_elem,size_elem);

	ct_high=0;
	ct_low=num_elem-1;

	for(i=0; i<num_elem; i++)
	{
		if(get_bit(&dat[size_elem*i],bit))
		{
			memcpy(
			    &buf[bit%2][ct_high*size_elem],
			    &dat[size_elem*i],
			    size_elem);
			ct_high++;
		}
		else
		{
			memcpy(
			    &buf[bit%2][ct_low*size_elem],
			    &dat[size_elem*i],
			    size_elem);
			ct_low--;
		}
	}

	high=ct_high;
	low=ct_low;
	bit++;


	while(bit<(8*size_elem))
	{
		ct_high=0;
		ct_low=num_elem-1;

		for(i=0; i<high; i++)
		{
			if(get_bit(&buf[(bit+1)%2][size_elem*i],bit))
			{
				memcpy(
				    &buf[bit%2][ct_high*size_elem],
				    &buf[(bit+1)%2][size_elem*i],
				    size_elem);
				ct_high++;
			}
			else
			{
				memcpy(
				    &buf[bit%2][ct_low*size_elem],
				    &buf[(bit+1)%2][size_elem*i],
				    size_elem);
				ct_low--;
			}
		}

		for(i=num_elem-1; i>low; i--)
		{
			if(get_bit(&buf[(bit+1)%2][size_elem*i],bit))
			{
				memcpy(
				    &buf[bit%2][ct_high*size_elem],
				    &buf[(bit+1)%2][size_elem*i],
				    size_elem);
				ct_high++;
			}
			else
			{
				memcpy(
				    &buf[bit%2][ct_low*size_elem],
				    &buf[(bit+1)%2][size_elem*i],
				    size_elem);
				ct_low--;
			}
		}

		high=ct_high;
		low=ct_low;
		bit++;
	}

	nfree(buf[bit%2]);

	return (void *)buf[(bit+1)%2];
}

