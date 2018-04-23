import { BrowserModule } from '@angular/platform-browser';
import { FormsModule, ReactiveFormsModule } from '@angular/forms';
import { LOCALE_ID, NgModule } from '@angular/core';
import { HttpClientModule, HTTP_INTERCEPTORS } from '@angular/common/http';

import { RoutingModule } from './routing/routing.module';
import { DstoreModule } from './dstore/dstore.module';

import { Locale } from './utils/locale';

import { AppService } from './services/app.service';
import { CategoryService } from './services/category.service';
import { SectionService } from './services/section.service';
import { DownloadService } from './services/download.service';
import { AuthService } from './services/auth.service';
import { AuthGuardService } from './services/auth-guard.service';
import { CommentService } from './services/comment.service';

import { AppComponent } from './app.component';
import { AppDetailComponent } from './components/app-detail/app-detail.component';
import { SideNavComponent } from './components/side-nav/side-nav.component';
import { CategoryComponent } from './components/category/category.component';
import { IndexComponent } from './components/index/index.component';
import { DownloadComponent } from './components/download/download.component';
import { UpdateComponent } from './components/update/update.component';
import { UninstallComponent } from './components/uninstall/uninstall.component';
import { RankingComponent } from './components/ranking/ranking.component';
import { AppListComponent } from './components/app-list/app-list.component';
import { AppTitleComponent } from './components/app-title/app-title.component';
import { AppCommentComponent } from './components/app-comment/app-comment.component';
import { StoreService } from './services/store.service';

@NgModule({
  declarations: [
    AppComponent,
    AppDetailComponent,
    CategoryComponent,
    DownloadComponent,
    UninstallComponent,
    UpdateComponent,
    SideNavComponent,
    CategoryComponent,
    IndexComponent,
    DownloadComponent,
    UpdateComponent,
    UninstallComponent,
    RankingComponent,
    AppListComponent,
    AppTitleComponent,
    AppCommentComponent,
  ],
  imports: [
    BrowserModule,
    FormsModule,
    ReactiveFormsModule,
    HttpClientModule,
    DstoreModule,
    RoutingModule,
  ],
  providers: [
    AppService,
    CategoryService,
    SectionService,
    DownloadService,
    AuthGuardService,
    AuthService,
    StoreService,
    CommentService,
    {
      provide: LOCALE_ID,
      useValue: Locale.getPcp47Locale(),
    },
  ],
  bootstrap: [AppComponent],
})
export class AppModule {}