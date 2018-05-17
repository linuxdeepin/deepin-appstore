import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable, of, concat } from 'rxjs';
import { map, share, retry, tap, shareReplay, catchError } from 'rxjs/operators';
import { throttle } from 'lodash';

import { BaseService } from '../dstore/services/base.service';
import { Category as DstoreCategory } from '../dstore/services/category.service';

@Injectable()
export class CategoryService {
  server = BaseService.serverHosts.operationServer;
  constructor(private http: HttpClient) {}

  list = throttle(this.getList, 1000 * 60);
  private getList(): Observable<Category[]> {
    return this.http.get<CustomCategory[]>(`${this.server}/api/blob/category`).pipe(
      retry(3),
      map(ccs => {
        if (ccs.length === 0) {
          return makeDefaultCategory();
        }
        return ccs.filter(c => c.show).map((c, index) => ({
          id: index.toString(),
          title: c.name,
          icon: c.icon.map(i => this.server + '/images/' + i),
          apps: c.apps,
        }));
      }),
      catchError(() => {
        console.log('getList err');
        return of(makeDefaultCategory());
      }),
      shareReplay(),
    );
  }
}
export interface Category {
  id: string;
  title: string;
  icon: string[];
  apps?: string[];
}

interface CustomCategory {
  name: '';
  icon: string[];
  show: boolean;
  apps: string[];
}
export function makeDefaultCategory(): Category[] {
  return [
    'internet',
    'office',
    'development',
    'reading',
    'graphics',
    'game',
    'music',
    'system',
    'video',
    'chat',
    'others',
  ].map(c => ({
    id: c,
    title: c,
    icon: [`/assets/category/${c}.svg`, `/assets/category/${c}_active.svg`],
  }));
}
const defaultCategory = [];