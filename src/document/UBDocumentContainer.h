/*
 * Copyright (C) 2015-2022 Département de l'Instruction Publique (DIP-SEM)
 *
 * Copyright (C) 2013 Open Education Foundation
 *
 * Copyright (C) 2010-2013 Groupement d'Intérêt Public pour
 * l'Education Numérique en Afrique (GIP ENA)
 *
 * This file is part of OpenBoard.
 *
 * OpenBoard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3 of the License,
 * with a specific linking exception for the OpenSSL project's
 * "OpenSSL" library (or with modified versions of it that use the
 * same license as the "OpenSSL" library).
 *
 * OpenBoard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBoard. If not, see <http://www.gnu.org/licenses/>.
 */




#ifndef UBDOCUMENTCONTAINER_H_
#define UBDOCUMENTCONTAINER_H_

#include <QtGui>
#include "UBDocumentProxy.h"

class UBDocumentContainer : public QObject
{
    Q_OBJECT

    public:
        UBDocumentContainer(QObject * parent = 0);
        virtual ~UBDocumentContainer();

        void setDocument(std::shared_ptr<UBDocumentProxy> document, bool forceReload = false);
        void pureSetDocument(std::shared_ptr<UBDocumentProxy> document) {mCurrentDocument = document;}

        std::shared_ptr<UBDocumentProxy> selectedDocument(){return mCurrentDocument;}
        int pageCount() const{return mCurrentDocument->pageCount();}

        static int pageFromSceneIndex(int sceneIndex);
        static int sceneIndexFromPage(int sceneIndex);

        void duplicatePage(int index);
        void deletePages(QList<int>& pageIndexes);
        void addPage(int index);

    public slots:
        void moveSceneToIndex(std::shared_ptr<UBDocumentProxy> proxy, int source, int target);

    private:
        std::shared_ptr<UBDocumentProxy> mCurrentDocument;

    signals:
        void documentSet(std::shared_ptr<UBDocumentProxy> document);
};


#endif /* UBDOCUMENTPROXY_H_ */
