/** \file
 * Defines the allocator interface as used by the KDTree class.
 *
 * \author Martin F. Krafft <krafft@ailab.ch>
 * \date $Date: 2004/11/15 17:40:32 $
 * \version $Revision: 1.6 $
 */

#ifndef INCLUDE_KDTREE_ALLOCATOR_HPP
#define INCLUDE_KDTREE_ALLOCATOR_HPP

#include <cstddef>
#include "kdtree/node.h"

namespace KDTree
{

   template <typename _Tp, typename _Alloc>
      class _Alloc_base
      {
        public:
         typedef __Node<_Tp> _Node;
         typedef typename _Node::_Base_ptr _Base_ptr;
         typedef _Alloc allocator_type;

         _Alloc_base(allocator_type const& __A)
            : _M_node_allocator(__A) {}

         allocator_type get_allocator() const
            {
               return _M_node_allocator;
            }

        protected:
         allocator_type _M_node_allocator;
      
         _Node* _M_allocate_node()
            {
               return _M_node_allocator.allocate(1);
            }

         void _M_deallocate_node(_Node* const __P)
            {
               return _M_node_allocator.deallocate(__P, 1);
            }

         void _M_construct_node(_Node* __p, _Tp const __V = _Tp(),
                                _Base_ptr const __PARENT = NULL,
                                _Base_ptr const __LEFT = NULL,
                                _Base_ptr const __RIGHT = NULL)
            {
               new (__p) _Node(__V, __PARENT, __LEFT, __RIGHT);
            }

         void _M_destroy_node(_Node* __p)
            {
               _M_node_allocator.destroy(__p);
            }
      };

} // namespace KDTree

#endif // include guard

/* COPYRIGHT --
 *
 * This file is part of libkdtree++, a C++ template KD-Tree sorting container.
 * libkdtree++ is (c) 2004 Martin F. Krafft <krafft@ailab.ch>
 * and distributed under the terms of the Artistic Licence.
 * See the ./COPYING file in the source tree root for more information.
 *
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES
 * OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
