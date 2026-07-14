package com.p1.backend.controller;

import com.p1.backend.dto.MatchStatusResponse;
import com.p1.backend.interceptor.JwtAuthInterceptor;
import com.p1.backend.service.MatchmakingService;
import jakarta.servlet.http.HttpServletRequest;
import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RestController;

@RestController
@RequestMapping("/api/match")
public class MatchController {

    private final MatchmakingService matchmakingService;

    public MatchController(MatchmakingService matchmakingService) {
        this.matchmakingService = matchmakingService;
    }

    @PostMapping("/queue")
    public MatchStatusResponse joinQueue(HttpServletRequest request) {
        return matchmakingService.joinQueue(resolveUsername(request));
    }

    @GetMapping("/status")
    public MatchStatusResponse status(HttpServletRequest request) {
        return matchmakingService.getStatus(resolveUsername(request));
    }

    private String resolveUsername(HttpServletRequest request) {
        return (String) request.getAttribute(JwtAuthInterceptor.AUTHENTICATED_USERNAME_ATTRIBUTE);
    }
}
