package com.p1.backend.dto;

// 매칭 대기열 참가 요청 바디 — heroId는 지금 매칭 로직에서 전혀 쓰이지 않는다(누구와 매칭되는지는
// 히어로와 무관). 클라이언트가 큐에 들어간 시점에 어떤 히어로를 골랐는지 기록만 해두기 위한 필드.
public record QueueRequest(String heroId) {
}
