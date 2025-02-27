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

#include "osg/osgSceneGraph/LineOfSight.h"

#include <osg/Notify>

#include <osgDB/ReadFile>

using namespace osgSim;

DatabaseCacheReadCallback::DatabaseCacheReadCallback()
{
    _maxNumFilesToCache = 2000;
}

void DatabaseCacheReadCallback::clearDatabaseCache()
{
    OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
    _filenameSceneMap.clear();
}

void DatabaseCacheReadCallback::pruneUnusedDatabaseCache()
{
}

osg::Node* DatabaseCacheReadCallback::readNodeFile(const std::string& filename)
{
    // first check to see if file is already loaded.
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        
        FileNameSceneMap::iterator itr = _filenameSceneMap.find(filename);
        if (itr != _filenameSceneMap.end())
        {
            osg::notify(osg::NOTICE)<<"Getting from cache "<<filename<<std::endl;
        
            return itr->second.get();
        }
    }
    
    // now load the file.
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filename);
    
    // insert into the cache.
    if (node.valid())
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        
        if (_filenameSceneMap.size() < _maxNumFilesToCache)
        {
            osg::notify(osg::NOTICE)<<"Inserting into cache "<<filename<<std::endl;

            _filenameSceneMap[filename] = node;
        }
        else
        {
            // for time being implement a crude search for a candidate to chuck out from the cache.
            for(FileNameSceneMap::iterator itr = _filenameSceneMap.begin();
                itr != _filenameSceneMap.end();
                ++itr)
            {
                if (itr->second->referenceCount()==1)
                {
                    osg::notify(osg::NOTICE)<<"Erasing "<<itr->first<<std::endl;
                    // found a node which is only referenced in the cache so we can disgard it
                    // and know that the actual memory will be released.
                    _filenameSceneMap.erase(itr);
                    break;
                }
            }
            osg::notify(osg::NOTICE)<<"And the replacing with "<<filename<<std::endl;
            _filenameSceneMap[filename] = node;
        }
    }
    
    return node.release();
}

LineOfSight::LineOfSight()
{
    setDatabaseCacheReadCallback(new DatabaseCacheReadCallback);
}

void LineOfSight::clear()
{
    _LOSList.clear();
}

unsigned int LineOfSight::addLOS(const osg::Vec3d& start, const osg::Vec3d& end)
{
    unsigned int index = _LOSList.size();
    _LOSList.push_back(LOS(start,end));
    return index;
}

void LineOfSight::computeIntersections(osg::Node* scene)
{
    osg::ref_ptr<osgUtil::IntersectorGroup> intersectorGroup = new osgUtil::IntersectorGroup();

    for(LOSList::iterator itr = _LOSList.begin();
        itr != _LOSList.end();
        ++itr)
    {
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(itr->_start, itr->_end);
        intersectorGroup->addIntersector( intersector.get() );
    }
    
    _intersectionVisitor.reset();
    _intersectionVisitor.setIntersector( intersectorGroup.get() );
    
    scene->accept(_intersectionVisitor);
    
    unsigned int index = 0;
    osgUtil::IntersectorGroup::Intersectors& intersectors = intersectorGroup->getIntersectors();
    for(osgUtil::IntersectorGroup::Intersectors::iterator intersector_itr = intersectors.begin();
        intersector_itr != intersectors.end();
        ++intersector_itr, ++index)
    {
        osgUtil::LineSegmentIntersector* lsi = dynamic_cast<osgUtil::LineSegmentIntersector*>(intersector_itr->get());
        if (lsi)
        {
            Intersections& intersectionsLOS = _LOSList[index]._intersections;
            _LOSList[index]._intersections.clear();
            
            osgUtil::LineSegmentIntersector::Intersections& intersections = lsi->getIntersections();
            
            for(osgUtil::LineSegmentIntersector::Intersections::iterator itr = intersections.begin();
                itr != intersections.end();
                ++itr)
            {
                const osgUtil::LineSegmentIntersector::Intersection& intersection = *itr;
                intersectionsLOS.push_back( intersection.localIntersectionPoint * (*intersection.matrix) );
            }
        }
    }
    
}

LineOfSight::Intersections LineOfSight::computeIntersections(osg::Node* scene, const osg::Vec3d& start, const osg::Vec3d& end)
{
    LineOfSight los;
    unsigned int index = los.addLOS(start,end);
    los.computeIntersections(scene);
    return los.getIntersections(index);
}

void LineOfSight::setDatabaseCacheReadCallback(DatabaseCacheReadCallback* dcrc)
{
    _dcrc = dcrc;
    _intersectionVisitor.setReadCallback(dcrc);
}
