//
//   ____                ___ _   _____ 
//  / __ \___  ___ ___  / _ \ | / / _ \
// / /_/ / _ \/ -_) _ \/ ___/ |/ / // /
// \____/ .__/\__/_//_/_/   |___/____/ 
//     /_/  https://github.com/ishani/OpenPVD
// 
// some parts of the physx lib expect a functioning Foundation instance to be around so that memory allocation and
// logging is hooked up; instance an Op::Foundation to get the basics running
//

#pragma once

namespace physx { class PxFoundation; }

namespace Op
{
    struct Foundation
    {
        Foundation();
        ~Foundation();

        physx::PxFoundation* m_foundation;
    };
}