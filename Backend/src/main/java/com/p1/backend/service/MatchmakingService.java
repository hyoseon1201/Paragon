package com.p1.backend.service;

import com.p1.backend.dto.MatchStatus;
import com.p1.backend.dto.MatchStatusResponse;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.stereotype.Service;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentLinkedQueue;

// 인메모리 매치메이킹 — Redis는 나중에(다중 백엔드 인스턴스로 확장할 때) 붙일 예정이라
// 지금은 단일 프로세스 전제로 동시성 컬렉션 + synchronized 블록만으로 충분하다.
// 오늘 MVP 범위: 실제 매치서버를 동적으로 띄우지 않고, 미리 정해둔 고정 Arena 서버 주소(match.server-address)를
// 대기열 인원이 다 찼을 때 배정한다.
@Service
public class MatchmakingService {

    private final int requiredPlayers;
    private final String serverAddress;

    // 대기열 순서 보장용 큐 + 중복 참가 방지용 Set. 두 자료구조의 정합성은 tryFormMatch()를 항상
    // joinQueue()의 synchronized 블록 안에서만 호출하는 것으로 보장한다.
    private final ConcurrentLinkedQueue<String> queue = new ConcurrentLinkedQueue<>();
    private final Set<String> queuedUsernames = ConcurrentHashMap.newKeySet();

    // 매칭 완료된 유저 → 배정된 서버 주소. /status 폴링이 여기서 결과를 읽어간다.
    private final ConcurrentHashMap<String, String> matchedUsers = new ConcurrentHashMap<>();

    public MatchmakingService(@Value("${match.required-players}") int requiredPlayers,
                               @Value("${match.server-address}") String serverAddress) {
        this.requiredPlayers = requiredPlayers;
        this.serverAddress = serverAddress;
    }

    public synchronized MatchStatusResponse joinQueue(String username) {
        // remove(): 매칭 결과는 클라이언트에게 한 번 전달되면 소비된다. get()으로 남겨두면
        // 예전에 매칭됐던 유저가 재접속 후 다시 큐에 들어갈 때 대기열 인원 체크 없이
        // 즉시 MATCHED가 재반환되는 버그가 생긴다(실제로 발생했던 문제).
        String existingMatch = matchedUsers.remove(username);
        if (existingMatch != null) {
            return new MatchStatusResponse(MatchStatus.MATCHED, existingMatch);
        }

        if (queuedUsernames.add(username)) {
            queue.add(username);
        }

        tryFormMatch();

        String matched = matchedUsers.remove(username);
        if (matched != null) {
            return new MatchStatusResponse(MatchStatus.MATCHED, matched);
        }
        return new MatchStatusResponse(MatchStatus.WAITING, null);
    }

    // 대기열 이탈. 이미 매칭이 성사된 뒤라면(이탈 요청이 도착하기 직전에 매칭이 형성된 레이스) 취소할
    // 수 없으므로 그대로 MATCHED를 돌려준다 — 호출부가 이걸 "매칭 성사"로 처리해야 한다.
    public synchronized MatchStatusResponse leaveQueue(String username) {
        String existingMatch = matchedUsers.remove(username);
        if (existingMatch != null) {
            return new MatchStatusResponse(MatchStatus.MATCHED, existingMatch);
        }

        if (queuedUsernames.remove(username)) {
            queue.remove(username);
        }

        return new MatchStatusResponse(MatchStatus.NOT_QUEUED, null);
    }

    public MatchStatusResponse getStatus(String username) {
        String matched = matchedUsers.remove(username);
        if (matched != null) {
            return new MatchStatusResponse(MatchStatus.MATCHED, matched);
        }
        if (queuedUsernames.contains(username)) {
            return new MatchStatusResponse(MatchStatus.WAITING, null);
        }
        return new MatchStatusResponse(MatchStatus.NOT_QUEUED, null);
    }

    private void tryFormMatch() {
        while (queue.size() >= requiredPlayers) {
            List<String> group = new ArrayList<>(requiredPlayers);
            for (int i = 0; i < requiredPlayers; i++) {
                String next = queue.poll();
                if (next == null) {
                    break;
                }
                queuedUsernames.remove(next);
                group.add(next);
            }
            for (String member : group) {
                matchedUsers.put(member, serverAddress);
            }
        }
    }
}
