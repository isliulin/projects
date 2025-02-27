/* -*-c++-*- OpenSceneGraph - Copyright (C) 1998-2006 Robert Osfield 
 *
 * This library is open source and may be redistributed and/or modified under  
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or 
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * OpenSceneGraph Public License for more details.
*/

#ifndef OSGSIM_LINEOFSIGHT
#define OSGSIM_LINEOFSIGHT 1

#include "osg/osgSceneGraph/IntersectionVisitor.h"

#include <osgSim/Export>

namespace osgSim {

class OSGSIM_EXPORT DatabaseCacheReadCallback : public osgUtil::IntersectionVisitor::ReadCallback
{
    public:
        DatabaseCacheReadCallback();
        
        void setMaximumNumOfFilesToCache(unsigned int maxNumFilesToCache) { _maxNumFilesToCache = maxNumFilesToCache; }
        unsigned int  getMaximumNumOfFilesToCache() const { return _maxNumFilesToCache; }
        
        void clearDatabaseCache();
        
        void pruneUnusedDatabaseCache();

        virtual osg::Node* readNodeFile(const std::string& filename);
        
    protected:
    
        typedef std::map<std::string, osg::ref_ptr<osg::Node> > FileNameSceneMap;
        
        unsigned int _maxNumFilesToCache;
        OpenThreads::Mutex  _mutex;
        FileNameSceneMap    _filenameSceneMap;
};

/** Helper class for setting up and aquiring line of sight intersections with terrain.
  * Supports automatic paging in of PagedLOD tiles. */
class OSGSIM_EXPORT LineOfSight
{
    public :
    
        LineOfSight();

        /** Clear the internal LOS List so it contains no line of sight tests.*/
        void clear();
    
        /** Add a line of sight test, consisting of start and end point. Returns the index number of the newly adding LOS test.*/
        unsigned int addLOS(const osg::Vec3d& start, const osg::Vec3d& end);
        
        /** Get the number of line of sight tests.*/
        unsigned int getNumLOS() const { return _LOSList.size(); }
        
        /** Set the start point of signel line of sight test.*/
        void setStartPoint(unsigned int i, const osg::Vec3d& start) { _LOSList[i]._start = start; }

        /** Get the start point of single line of sight test.*/
        const osg::Vec3d& getStartPoint(unsigned int i) const { return _LOSList[i]._start; }

        /** Set the end point of single line of sight test.*/
        void setEndPoint(unsigned int i, const osg::Vec3d& end) { _LOSList[i]._end = end; }

        /** Get the end point of single line of sight test.*/
        const osg::Vec3d& getEndPoint(unsigned int i) const { return _LOSList[i]._end; }
        
        typedef std::vector<osg::Vec3d> Intersections;

        /** Get the intersection points for a single line of sight test.*/
        const Intersections& getIntersections(unsigned int i) const  { return _LOSList[i]._intersections; }

        /** Compute the LOS intersections with the specified scene graph.
          * The results are all stored in the form of Intersections list, one per LOS test.*/
        void computeIntersections(osg::Node* scene);

        /** Compute the intersection between the specified scene graph and a single LOS start,end pair. Returns an IntersectionList, of all the points intersected.*/
        static Intersections computeIntersections(osg::Node* scene, const osg::Vec3d& start, const osg::Vec3d& end);
        
        
        /** Clear the database cache.*/
        void clearDatabaseCache() { if (_dcrc.valid()) _dcrc->clearDatabaseCache(); }

        /** Set the ReadCallback that does the reading of external PagedLOD models, and caching of loaded subgraphs.
          * Note, if you have mulitple LineOfSight or HeightAboveTerrain objects in use at one time then you should share a single
          * DatabaseCacheReadCallback between all of them. */
        void setDatabaseCacheReadCallback(DatabaseCacheReadCallback* dcrc);

        /** Get the ReadCallback that does the reading of external PagedLOD models, and caching of loaded subgraphs.*/
        DatabaseCacheReadCallback* getDatabaseCacheReadCallback() { return _dcrc.get(); }
        
        /** Get the IntersectionVistor that does the intersection traversal over the scene.
          * Note, if you want to customized the traversal then you can use the IntersectionVisitor's method to alter its behavior. */
        osgUtil::IntersectionVisitor& getIntersectionVisitor() { return _intersectionVisitor; }

    protected :
    
        struct LOS
        {
            LOS(const osg::Vec3d& start, const osg::Vec3d& end):
                _start(start),
                _end(end) {}
                
            
            osg::Vec3d      _start;
            osg::Vec3d      _end;
            Intersections   _intersections;
        };
        
        typedef std::vector<LOS> LOSList;
        LOSList _LOSList;
        
        osg::ref_ptr<DatabaseCacheReadCallback> _dcrc;
        osgUtil::IntersectionVisitor            _intersectionVisitor;

};

}

#endif
