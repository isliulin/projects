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

#ifndef OSGSIM_HEIGHTABOVETERRAIN
#define OSGSIM_HEIGHTABOVETERRAIN 1

#include "osg/osgSceneGraph/IntersectionVisitor.h"

// include so we can get access to the DatabaseCacheReadCallback
#include "osg/osgSceneGraph/LineOfSight.h"

namespace osgSim {

/** Helper class for setting up and aquiring height above terrain intersections with terrain.*/
class OSGSIM_EXPORT HeightAboveTerrain
{
    public :

    
        HeightAboveTerrain();
        
    
        /** Clear the internal HAT List so it contains no height above terrain tests.*/
        void clear();
        
        /** Add a height above terrain test point in the CoordinateFrame.*/
        unsigned int addPoint(const osg::Vec3d& point);

        /** Get the number of height above terrain tests.*/
        unsigned int getNumPoints() const { return _HATList.size(); }
        
        /** Set the source point of single height above terrain test.*/
        void setPoint(unsigned int i, const osg::Vec3d& point) { _HATList[i]._point = point; }

        /** Get the source point of single height above terrain test.*/
        const osg::Vec3d& getPoint(unsigned int i) const { return _HATList[i]._point; }

        /** Get the intersection height for a single height above terrain test.
          * Note, you must call computeIntersections(..) before you can querry the HeightAboveTerrain. 
          * If no intersections are found then height returned will be the height above mean sea level. */
        double getHeightAboveTerrain(unsigned int i) const  { return _HATList[i]._hat; }

        /** Set the lowest height that the should be tested for.
          * Defaults to -1000, i.e. 1000m below mean sea level. */
        void setLowestHeight(double lowestHeight) { _lowestHeight = lowestHeight; }

        /** Get the lowest height that the should be tested for.*/
        double getLowestHeight() const { return _lowestHeight; }
        
        /** Compute the HAT intersections with the specified scene graph.
          * The results are all stored in the form of a single height above terrain value per HAT test.
          * Note, if the topmost node is a CoordinateSystemNode then the input points are assumed to be geocentric,
          * with the up vector defined by the EllipsoidModel attached to the CoordinateSystemNode.
          * If the topmost node is not a CoordinateSystemNode then a local coordinates frame is assumed, with a local up vector. */
        void computeIntersections(osg::Node* scene);

        /** Compute the vertical distance between the specified scene graph and a single HAT point. .*/
        static double computeHeightAboveTerrain(osg::Node* scene, const osg::Vec3d& point);
        
        
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
    
        struct HAT
        {
            HAT(const osg::Vec3d& point):
                _point(point),
                _hat(0.0) {}
                
            osg::Vec3d      _point;
            double          _hat;
        };
        
        typedef std::vector<HAT> HATList;
        

        double                                  _lowestHeight;
        HATList                                 _HATList;

        
        osg::ref_ptr<DatabaseCacheReadCallback> _dcrc;
        osgUtil::IntersectionVisitor            _intersectionVisitor;


};

}

#endif
