//
//  Grad.h
//  ttcr_u
//
//  Created by Bernard Giroux on 2014-03-04.
//  Copyright (c) 2014 Bernard Giroux. All rights reserved.
//

/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef ttcr_u_Grad_h
#define ttcr_u_Grad_h


#include <cmath>
#include <set>

#include <Eigen/Dense>

#include "ttcr_t.h"
#include "Node.h"

namespace ttcr {
    
    template <typename T>
    class Grad2D {
    public:
        Grad2D() {}
        
        sxz<T> grad(const Node<T> &n0,
                    const Node<T> &n1,
                    const Node<T> &n2,
                    const size_t nt);
        sxz<T> ls_grad(const Node<T> &n0,
                       const Node<T> &n1,
                       const Node<T> &n2,
                       const size_t nt);
        
    private:
        sxz<T> g;
        Eigen::Matrix<T, 3, 2> A;
        Eigen::Matrix<T, 2, 1> x;
        Eigen::Matrix<T, 3, 1> b;
    };
    
    template <typename T>
    sxz<T> Grad2D<T>::grad(const Node<T> &n0,
                           const Node<T> &n1,
                           const Node<T> &n2,
                           const size_t nt) {
        T dx1 = n1.getX() - n0.getX();
        T dz1 = n1.getZ() - n0.getZ();
        T dt1 = n0.getTT(nt) - n1.getTT(nt);
        T dx2 = n2.getX() - n0.getX();
        T dz2 = n2.getZ() - n0.getZ();
        T dt2 = n0.getTT(nt) - n2.getTT(nt);
        
        if ( dx1 == 0.0 ) {
            g.z = dt1/dz1;
            g.x = (dt2 - dz2*g.z) / dx2;
        } else if ( dz2 == 0.0 ) {
            g.x = dt2/dx2;
            g.z = (dt1 - dx1*g.x) / dz1;
        } else {
            g.z = (dx1*dt2 - dx2*dt1) / (dx1*dz2);
            g.x = (dt1 - dz1*g.z) / dx1;
        }
        return g;
    }
    
    template <typename T>
    sxz<T> Grad2D<T>::ls_grad(const Node<T> &n0,
                              const Node<T> &n1,
                              const Node<T> &n2,
                              const size_t nt) {
        
        // find centroid of triangle
        sxz<T> cent = {(n0.getX()+n1.getX()+n2.getX())/static_cast<T>(3.),
            (n0.getZ()+n1.getZ()+n2.getZ())/static_cast<T>(3.)};
        
        // time at centroid is inverse distance weeighted
        T w = 1./sqrt( (n0.getX()-cent.x)*(n0.getX()-cent.x) + (n0.getZ()-cent.z)*(n0.getZ()-cent.z) );
        T t = w*n0.getTT(nt);
        T den = w;
        w = 1./sqrt( (n1.getX()-cent.x)*(n1.getX()-cent.x) + (n1.getZ()-cent.z)*(n1.getZ()-cent.z) );
        t += w*n1.getTT(nt);
        den += w;
        w = 1./sqrt( (n2.getX()-cent.x)*(n2.getX()-cent.x) + (n2.getZ()-cent.z)*(n2.getZ()-cent.z) );
        t += w*n2.getTT(nt);
        den += w;
        
        t /= den;
        
        A(0,0) = n0.getX() - cent.x;
        A(0,1) = n0.getZ() - cent.z;
        b(0,0) = t - n0.getTT(nt);
        
        A(1,0) = n1.getX() - cent.x;
        A(1,1) = n1.getZ() - cent.z;
        b(1,0) = t - n1.getTT(nt);
        
        A(2,0) = n2.getX() - cent.x;
        A(2,1) = n2.getZ() - cent.z;
        b(2,0) = t - n2.getTT(nt);
        
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        
        g.x = x[0];
        g.z = x[1];
        return g;
    }
    
    
    // high order
    template <typename T, typename NODE>
    class Grad2D_ho {
    public:
        Grad2D_ho() {}
        
        sxz<T> ls_grad(const std::set<NODE*> &nodes,
                       const size_t nt);
        
    private:
        sxz<T> g;
        Eigen::Matrix<T, Eigen::Dynamic, 5> A;
        Eigen::Matrix<T, 5, 1> x;
        Eigen::Matrix<T, Eigen::Dynamic, 1> b;
    };
    
    
    template <typename T, typename NODE>
    sxz<T> Grad2D_ho<T,NODE>::ls_grad(const std::set<NODE*> &nodes,
                                      const size_t nt) {
        
        assert(nodes.size()>=5);
        
        sxz<T> cent = { 0.0, 0.0 };
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            cent.x += (*n)->getX();
            cent.z += (*n)->getZ();
        }
        T den = 1./nodes.size();
        cent.x *= den;
        cent.z *= den;
        
        T t = 0.0;
        den = 0.0;
        
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            T w = 1./sqrt(((*n)->getX()-cent.x)*((*n)->getX()-cent.x) +
                          ((*n)->getZ()-cent.z)*((*n)->getZ()-cent.z) );
            t += w*(*n)->getTT(nt);
            den += w;
        }
        t /= den;
        
        A.resize( nodes.size(), 5 );
        b.resize( nodes.size(), 1 );
        
        size_t i=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            T dx = (*n)->getX()-cent.x;
            T dz = (*n)->getZ()-cent.z;
            
            A(i,0) = dx;
            A(i,1) = dz;
            A(i,2) = dx*dx;
            A(i,3) = dz*dz;
            A(i,4) = dx*dz;
            
            b(i,0) = t - (*n)->getTT(nt);
            i++;
        }
        
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        
        //	Eigen::Matrix<T, Eigen::Dynamic, 1> e = b-A*x;
        
        g.x = x[0];
        g.z = x[1];
        
        return g;
    }
    
    
    template <typename T, typename NODE>
    class Grad3D {
    public:
        Grad3D() {}
        
        sxyz<T> ls_grad(const NODE &n0,
                        const NODE &n1,
                        const NODE &n2,
                        const NODE &n3,
                        const size_t nt);
        sxyz<T> ABM_grad(const NODE &n0,
                        const NODE &n1,
                        const NODE &n2,
                        const NODE &n3,
                        const size_t nt);
        sxyz<T> ls_grad(const NODE &n0,
                        const NODE &n1,
                        const NODE &n2,
                        const NODE &n3,
                        const size_t nt,
                        const sxyz<T> &nod);
        sxyz<T> Interp_grad(const std::set<NODE*> &nodes,
                            const size_t nt, const sxyz<T> & nod);
        
        sxyz<T> ls_grad(const std::set<NODE*> &nodes,
                        const size_t nt,const sxyz<T> & nod);
        
        sxyz<T> RM4D_grad(const std::set<NODE*> &nodes,
                        const size_t nt,const sxyz<T> & nod);
        sxyz<T> FReconstraction_grad(const std::set<NODE*> &nodes,
                          const size_t nt,const sxyz<T> & nod);
        
        sxyz<T> ls_grad(const std::set<NODE*> &nodes,
                        const size_t nt,const T &timeNode,const sxyz<T> & nod);
        
        Eigen::Matrix<T, 3, 1> solveSystem(const Eigen::Matrix<T, Eigen::Dynamic, 8>& A,
                                           const Eigen::Matrix<T, Eigen::Dynamic, 1>& b,
                                           const T & Norm) const;
    private:
        sxyz<T> g;
        
        Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> A;
        Eigen::Matrix<T, Eigen::Dynamic, 1> x;
        Eigen::Matrix<T, Eigen::Dynamic, 1> b;
    };
    template <typename T, typename NODE>
    Eigen::Matrix<T,3, 1> Grad3D<T,NODE>::solveSystem(const Eigen::Matrix<T, Eigen::Dynamic, 8>& A,
                                                        const Eigen::Matrix<T, Eigen::Dynamic, 1>& b,const T & Norm) const{
        size_t n=A.rows();
        Eigen::Matrix<T, 8, 1> x;
        Eigen::Matrix<T, Eigen::Dynamic, 3> J;
        J.resize(n+1, 3);
        J.block(0,0,n,3)=A;
        for (size_t i=0;i<3;++i){
            x(i,0)=0;
            J(n,i)=0;
        }
        Eigen::Matrix<T, Eigen::Dynamic, 1> r;
        r.resize(n+1,1);
        Eigen::Matrix<T, 3, 1> s;
        size_t nItration=20;
        double alpha=10;
        for(size_t i=0;i<nItration;++i){
            r.block(0,0,n,1)=A*x-b;
            r(n,0)=((x(0,0)*x(0,0)+x(1,0)*x(1,0)+x(2,0)*x(2,0))-Norm*Norm)*alpha;
            //std::cout<<"\n r: \n"<<r<<std::endl;
            J(n,0)=2*x(0,0)*alpha;
            J(n,1)=2*x(1,0)*alpha;
            J(n,2)=2*x(2,0)*alpha;
            s=(J.transpose()*J).inverse()*(J.transpose()*r);
            if ((s.norm()/x.norm())*100>0.01){
                x=x-s;
                //std::cout<<"\n x: \n"<<x<<std::endl;
            }else{
                break;
            }
        }
        return x;
    }
    
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::ABM_grad(const NODE &n0,
                                    const NODE &n1,
                                    const NODE &n2,
                                    const NODE &n3,
                                    const size_t nt) {
        
        
        A.resize( 3, 3 );
        b.resize( 3, 1 );
        x.resize(3,1);
        
        A(0,0) = n1.getX()-n0.getX();
        A(0,1) = n1.getY()-n0.getY();
        A(0,2) = n1.getZ()-n0.getZ();
        b(0,0) = n0.getTT(nt)-n1.getTT(nt);
        
        
        A(1,0) = n2.getX()-n0.getX();
        A(1,1) = n2.getY()-n0.getY();
        A(1,2) = n2.getZ()-n0.getZ();
        b(1,0) = n0.getTT(nt)-n2.getTT(nt);
        
        
        A(2,0) = n3.getX()-n0.getX();
        A(2,1) = n3.getY()-n0.getY();
        A(2,2) = n3.getZ()-n0.getZ();
        b(2,0) = n0.getTT(nt)-n3.getTT(nt);
        
        
        // solve Ax = b with least squares
        x = A.inverse()*b;
        
        //	Eigen::Matrix<T, 4, 1> e = b-A*x;
        
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        return g;
    }
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::ls_grad(const NODE &n0,
                                    const NODE &n1,
                                    const NODE &n2,
                                    const NODE &n3,
                                    const size_t nt) {
        
        
        A.resize( 4, 4 );
        b.resize( 4, 1 );
        x.resize(4,1);
        
        A(0,0) = n0.getX();
        A(0,1) = n0.getY();
        A(0,2) = n0.getZ();
        A(0,3) =1.0;
        b(0,0) = n0.getTT(nt);
        
        A(1,0) = n1.getX() ;
        A(1,1) = n1.getY() ;
        A(1,2) = n1.getZ() ;
        A(1,3) =1.0;
        b(1,0) = n1.getTT(nt);
        
        A(2,0) = n2.getX() ;
        A(2,1) = n2.getY() ;
        A(2,2) = n2.getZ() ;
        A(2,3) =1.0;
        b(2,0) = n2.getTT(nt);
        
        A(3,0) = n3.getX() ;
        A(3,1) = n3.getY() ;
        A(3,2) = n3.getZ();
        A(3,3) =1.0;
        b(3,0) = n3.getTT(nt);
        
        // solve Ax = b with least squares
        x = A.inverse()*b;
        
        //    Eigen::Matrix<T, 4, 1> e = b-A*x;
        
        g.x = -x[0];
        g.y = -x[1];
        g.z = -x[2];
        return g;
    }
    
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::ls_grad(const NODE &n0,
                                    const NODE &n1,
                                    const NODE &n2,
                                    const NODE &n3,
                                    const size_t nt,
                                    const sxyz<T> &nod) {
        
        sxyz<T> cent (nod.x, nod.y,nod.z);
        
        T w = 1./sqrt((n0.getX()-cent.x)*(n0.getX()-cent.x) +
                      (n0.getY()-cent.y)*(n0.getY()-cent.y) +
                      (n0.getZ()-cent.z)*(n0.getZ()-cent.z) );
        T t = w*n0.getTT(nt);
        T den = w;
        
        w = 1./sqrt((n1.getX()-cent.x)*(n1.getX()-cent.x) +
                    (n1.getY()-cent.y)*(n1.getY()-cent.y) +
                    (n1.getZ()-cent.z)*(n1.getZ()-cent.z) );
        t += w*n1.getTT(nt);
        den += w;
        
        w = 1./sqrt((n2.getX()-cent.x)*(n2.getX()-cent.x) +
                    (n2.getY()-cent.y)*(n2.getY()-cent.y) +
                    (n2.getZ()-cent.z)*(n2.getZ()-cent.z) );
        t += w*n2.getTT(nt);
        den += w;
        
        w = 1./sqrt((n3.getX()-cent.x)*(n3.getX()-cent.x) +
                    (n3.getY()-cent.y)*(n3.getY()-cent.y) +
                    (n3.getZ()-cent.z)*(n3.getZ()-cent.z) );
        t += w*n3.getTT(nt);
        den += w;
        
        t /= den;
        
        A.resize( 4, 3 );
        b.resize( 4, 1 );
        
        A(0,0) = n0.getX() - cent.x;
        A(0,1) = n0.getY() - cent.y;
        A(0,2) = n0.getZ() - cent.z;
        b(0,0) = t - n0.getTT(nt);
        
        A(1,0) = n1.getX() - cent.x;
        A(1,1) = n1.getY() - cent.y;
        A(1,2) = n1.getZ() - cent.z;
        b(1,0) =t - n1.getTT(nt);
        
        A(2,0) = n2.getX() - cent.x;
        A(2,1) = n2.getY() - cent.y;
        A(2,2) = n2.getZ() - cent.z;
        b(2,0) = t - n2.getTT(nt);
        
        A(3,0) = n3.getX() - cent.x;
        A(3,1) = n3.getY() - cent.y;
        A(3,2) = n3.getZ() - cent.z;
        b(3,0) = t - n3.getTT(nt);
        
        //solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        //x=solveSystem(A, b, 0.5);
        //	Eigen::Matrix<T, 4, 1> e = b-A*x;
        
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::ls_grad(const std::set<NODE*> &nodes,
                                    const size_t nt, const sxyz<T> & nod) {
        
        assert(nodes.size()>=4);
        
        sxyz<T> cent = { nod.x,nod.y, nod.z};

        
        T t = 0.0;
        T den = 0.0;
        
        int remove = 0;
        std::vector<T> d( nodes.size() );
        size_t nn=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            d[nn] = sqrt(((*n)->getX()-cent.x)*((*n)->getX()-cent.x) +
                         ((*n)->getY()-cent.y)*((*n)->getY()-cent.y) +
                         ((*n)->getZ()-cent.z)*((*n)->getZ()-cent.z) );
            if ( d[nn] == 0.0 ) {
                remove++;
                nn++;
                continue;
            }
            T w = 1./d[nn];
            t += w*(*n)->getTT(nt);
            den += w;
            nn++;
        }
        t /= den;
        Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> W;
        //W.setZero(nodes.size(),nodes.size());
        A.resize( nodes.size()-remove, 3 );
        b.resize( nodes.size()-remove, 1 );
        
        size_t i=0;
        nn=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            if ( d[nn] == 0.0 ) {
                nn++;
                continue;
            }
            A(i,0) = (*n)->getX()-cent.x;
            A(i,1) = (*n)->getY()-cent.y;
            A(i,2) = (*n)->getZ()-cent.z;
            //W(i,i)=1.0/sqrt(A(i,0)*A(i,0)+A(i,1)*A(i,1)+A(i,2)*A(i,2));
            b(i,0) = (t - (*n)->getTT(nt));
            i++;
            nn++;
        }
//        A=W*A;
//        b=W*b;
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        //	Eigen::Matrix<T, Eigen::Dynamic, 1> e = b-A*x;
        
        //	if ( isnan(x[0]) || isnan(x[1]) || isnan(x[2]) ) {
        //		g.x = g.y = g.z = 0;
        //	}
        
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::RM4D_grad(const std::set<NODE*> &nodes,
                                    const size_t nt, const sxyz<T> & nod) {
        
        assert(nodes.size()>=5);
        
        Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> W;
        W.setZero(nodes.size(),nodes.size());
        A.resize( nodes.size(), 4);
        b.resize( nodes.size(), 1 );
        
        size_t i=0;

        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            A(i,0) = (*n)->getX();
            A(i,1) = (*n)->getY();
            A(i,2) = (*n)->getZ();
            A(i,3)=1.0;
            W(i,i)=1.0/(((*n)->getX()-nod.x)*((*n)->getX()-nod.x)+
                            ((*n)->getY()-nod.y)*((*n)->getY()-nod.y)+
                            ((*n)->getZ()-nod.z)*((*n)->getZ()-nod.z));
            b(i,0) = - (*n)->getTT(nt);
            i++;
            
        }
        A=W*A;
        b=W*b;
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        //    Eigen::Matrix<T, Eigen::Dynamic, 1> e = b-A*x;
        
        //    if ( isnan(x[0]) || isnan(x[1]) || isnan(x[2]) ) {
        //        g.x = g.y = g.z = 0;
        //    }
        
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::FReconstraction_grad(const std::set<NODE*> &nodes,
                                      const size_t nt, const sxyz<T> & nod) {
        
        assert(nodes.size()>=8);
        
        Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> W;
        W.setZero(nodes.size(),nodes.size());
        A.resize( nodes.size(), 8);
        b.resize( nodes.size(), 1 );
        
        size_t i=0;
        
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            A(i,0) = 1.0;
            A(i,1) = (*n)->getX();
            A(i,2) = (*n)->getY();
            A(i,3)=(*n)->getZ();
            A(i,4)=(*n)->getX()*(*n)->getY();
            A(i,5)=(*n)->getX()*(*n)->getZ();
            A(i,6)=(*n)->getY()*(*n)->getZ();
            A(i,7)=(*n)->getX()*(*n)->getY()*(*n)->getZ();
            W(i,i)=pow(1.0/(((*n)->getX()-nod.x)*((*n)->getX()-nod.x)+
                        ((*n)->getY()-nod.y)*((*n)->getY()-nod.y)+
                        ((*n)->getZ()-nod.z)*((*n)->getZ()-nod.z)),2);
            b(i,0) = - (*n)->getTT(nt);
            i++;
            
        }
        A=W*A;
        b=W*b;
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);

        
        g.x = x[1]+x[4]*nod.y+x[5]*nod.z+x[7]*nod.z*nod.y;
        g.y = x[2]+x[4]*nod.x+x[6]*nod.z+x[7]*nod.z*nod.x;
        g.z = x[3]+x[5]*nod.x+x[6]*nod.y+x[7]*nod.y*nod.x;
        
        return g;
    }
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::ls_grad(const std::set<NODE*> & nodes,
                                    const size_t nt,const T & t ,const sxyz<T> & node) {
        
        assert(nodes.size()>=4);
        Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> W;
        W.setZero(nodes.size(),nodes.size());
        A.resize( nodes.size(), 3 );
        b.resize( nodes.size(), 1 );
        
        size_t i=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {

            A(i,0) = (*n)->getX()-node.x;
            A(i,1) = (*n)->getY()-node.y;
            A(i,2) = (*n)->getZ()-node.z;
            W(i,i)=1.0/((((*n)->getX()-node.x)*((*n)->getX()-node.x)+
            ((*n)->getY()-node.y)*((*n)->getY()-node.y)+
                        ((*n)->getZ()-node.z)*((*n)->getZ()-node.z)));
            b(i,0) = (t - (*n)->getTT(nt));
            i++;
        }
        A=W*A;
        b=W*b;
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        //    Eigen::Matrix<T, Eigen::Dynamic, 1> e = b-A*x;
        
        //    if ( isnan(x[0]) || isnan(x[1]) || isnan(x[2]) ) {
        //        g.x = g.y = g.z = 0;
        //    }
        
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
    template <typename T, typename NODE>
    sxyz<T> Grad3D<T,NODE>::Interp_grad(const std::set<NODE*> &nodes,
                                    const size_t nt, const sxyz<T> & nod) {
        
        assert(nodes.size()>=8);
        A.resize( nodes.size(), 8 );
        b.resize( nodes.size(), 1);
        x.resize(8,1);
        size_t i=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            A(i,0) = 1.0;
            A(i,1) = (*n)->getX();
            A(i,2) = (*n)->getY();
            A(i,3) = (*n)->getZ();
            A(i,4) = (*n)->getX()*(*n)->getY();
            A(i,5) = (*n)->getX()*(*n)->getZ();
            A(i,6) = (*n)->getY()*(*n)->getZ();
            A(i,7) = (*n)->getX()*(*n)->getY()*(*n)->getZ();
            b(i,0) = - (*n)->getTT(nt);
            i++;
        }
        
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        //    Eigen::Matrix<T, Eigen::Dynamic, 1> e = b-A*x;
        
        //    if ( isnan(x[0]) || isnan(x[1]) || isnan(x[2]) ) {
        //        g.x = g.y = g.z = 0;
        //    }
        
        g.x = x[1]+x[4]*nod.y+x[5]*nod.z+x[7]*nod.y*nod.z;
        g.y = x[2]+x[4]*nod.x+x[6]*nod.z+x[7]*nod.x*nod.z;
        g.z = x[3]+x[5]*nod.x+x[6]*nod.y+x[7]*nod.x*nod.y;
        
        return g;
    }
    
    // high order
    template <typename T, typename NODE>
    class Grad3D_ho {
    public:
        Grad3D_ho() {}
        
        sxyz<T> ls_grad(const std::set<NODE*> &nodes,
                        const size_t nt);
        sxyz<T> ls_grad(const std::set<NODE*> &nodes,
                        const size_t nt, const sxyz<T>& NOD);
        sxyz<T> ls_grad(const std::set<NODE*> &nodes,
                        const size_t nt, const T & t,const sxyz<T>& NOD);
        
    private:
        sxyz<T> g;
        Eigen::Matrix<T, Eigen::Dynamic, 9> A;
        Eigen::Matrix<T, 9, 1> x;
        Eigen::Matrix<T, Eigen::Dynamic, 1> b;
    };
    template <typename T, typename NODE>
    sxyz<T> Grad3D_ho<T,NODE>::ls_grad(const std::set<NODE*> &nodes,
                                       const size_t nt) {
        
        assert(nodes.size()>=9);
        
        sxyz<T> cent = { 0.0, 0.0, 0.0 };
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            cent.x += (*n)->getX();
            cent.y += (*n)->getY();
            cent.z += (*n)->getZ();
        }
        T den = 1./nodes.size();
        cent.x *= den;
        cent.y *= den;
        cent.z *= den;
        
        T t = 0.0;
        den = 0.0;
        
        int remove = 0;
        std::vector<T> d( nodes.size() );
        size_t nn=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            d[nn] = sqrt(((*n)->getX()-cent.x)*((*n)->getX()-cent.x) +
                         ((*n)->getY()-cent.y)*((*n)->getY()-cent.y) +
                         ((*n)->getZ()-cent.z)*((*n)->getZ()-cent.z) );
            if ( d[nn] == 0.0 ) {
                remove++;
                nn++;
                continue;
            }
            T w = 1./d[nn];
            t += w*(*n)->getTT(nt);
            den += w;
            nn++;
        }
        t /= den;
        
        A.resize( nodes.size()-remove, 9 );
        b.resize( nodes.size()-remove, 1 );
        
        size_t i=0;
        nn=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            if ( d[nn] == 0.0 ) {
                nn++;
                continue;
            }
            T dx = (*n)->getX()-cent.x;
            T dy = (*n)->getY()-cent.y;
            T dz = (*n)->getZ()-cent.z;
            
            A(i,0) = dx;
            A(i,1) = dy;
            A(i,2) = dz;
            A(i,3) = static_cast<T>(0.5*dx*dx);
            A(i,4) = static_cast<T>(0.5*dy*dy);
            A(i,5) = static_cast<T>(0.5*dz*dz);
            A(i,6) = dx*dy;
            A(i,7) = dx*dz;
            A(i,8) = dy*dz;
            
            b(i,0) = t-(*n)->getTT(nt);
            i++;
            nn++;
        }
        
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        
        //	Eigen::Matrix<T, Eigen::Dynamic, 1> e = b-A*x;
        
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
    template <typename T, typename NODE>
 
sxyz<T>  Grad3D_ho<T,NODE>::ls_grad(const std::set<NODE*> &nodes,
                                    const size_t nt,
                                    const sxyz<T>& nod){
        
        assert(nodes.size()>=9);
        T t = 0.0;
        T den = 0.0;;
        int remove = 0;
        std::vector<T> d( nodes.size() );
        size_t nn=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            d[nn]= sqrt(((*n)->getX()-nod.x)*((*n)->getX()-nod.x) +
                         ((*n)->getY()-nod.y)*((*n)->getY()-nod.y) +
                         ((*n)->getZ()-nod.z)*((*n)->getZ()-nod.z) );
            if ( d[nn] == 0.0 ) {
                remove++;
                nn++;
                continue;
            }
            T w = 1./d[nn];
            t += w*(*n)->getTT(nt);
            den += w;
            nn++;
        }
        t /= den;
        
        A.resize( nodes.size()-remove, 9 );
        b.resize( nodes.size()-remove, 1 );
        size_t i=0;
        nn=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            if ( d[nn] == 0.0 ) {
                nn++;
                continue;
            }
            T dx = (*n)->getX()-nod.x;
            T dy = (*n)->getY()-nod.y;
            T dz = (*n)->getZ()-nod.z;
            
            A(i,0) = dx;
            A(i,1) = dy;
            A(i,2) = dz;
            A(i,3) = static_cast<T>(0.5*dx*dx);
            A(i,4) = static_cast<T>(0.5*dy*dy);
            A(i,5) = static_cast<T>(0.5*dz*dz);
            A(i,6) = dx*dy;
            A(i,7) = dx*dz;
            A(i,8) = dy*dz;
//            
            //b(i,0) = pow(1000.0*t,2)-pow(((*n)->getTT(nt))*1000.0,2);// a verifie
           b(i,0) = (t-(*n)->getTT(nt));
            i++;
            nn++;
        }

        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
    template <typename T, typename NODE>
    
    sxyz<T>  Grad3D_ho<T,NODE>::ls_grad(const std::set<NODE*> &nodes,
                                        const size_t nt,const T& t,
                                        const sxyz<T>& nod){
        
        assert(nodes.size()>=9);
        A.resize( nodes.size(), 9 );
        b.resize( nodes.size(), 1 );
        Eigen::Matrix<T,Eigen::Dynamic,Eigen::Dynamic> W;
        W.setZero(nodes.size(),nodes.size());
        size_t i=0;
        for ( auto n=nodes.cbegin(); n!=nodes.cend(); ++n ) {
            T dx = (*n)->getX()-nod.x;
            T dy = (*n)->getY()-nod.y;
            T dz = (*n)->getZ()-nod.z;
            A(i,0) = dx;
            A(i,1) = dy;
            A(i,2) = dz;
            A(i,3) = static_cast<T>(0.5*dx*dx);
            A(i,4) = static_cast<T>(0.5*dy*dy);
            A(i,5) = static_cast<T>(0.5*dz*dz);
            A(i,6) = dx*dy;
            A(i,7) = dx*dz;
            A(i,8) = dy*dz;
            //
            b(i,0) = (t-(*n)->getTT(nt));
            i++;
        }
        // solve Ax = b with least squares
        x = A.jacobiSvd(Eigen::ComputeFullU | Eigen::ComputeFullV).solve(b);
        g.x = x[0];
        g.y = x[1];
        g.z = x[2];
        
        return g;
    }
  
}

#endif
